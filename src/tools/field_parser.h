/**
 * @file
 * @brief Utility to parse INIT-format field files
 *
 * @copyright Copyright (c) 2018-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_FIELD_PARSER_H
#define ALLPIX_FIELD_PARSER_H

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

#include "core/utils/log.h"
#include "core/utils/unit.h"

#include <cereal/archives/portable_binary.hpp>

#include <cereal/types/array.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <utility>

// Mime type version for APF files
#define APF_MIME_TYPE_VERSION 1

namespace allpix {

    /**
     * @brief Field quantities
     */
    enum class FieldQuantity : size_t {
        UNKNOWN = 0, ///< Unknown field quantity
        SCALAR = 1,  ///< Scalar field, i.e. one entry per field position
        VECTOR = 3,  ///< Vector field, i.e. three entries per field position
    };

    /**
     * @brief Type of file formats
     */
    enum class FileType {
        UNKNOWN = 0, ///< Unknown file format
        INIT,        ///< Legacy file format, values stored in plain-text ASCII
        APF,         ///< Binary Allpix Squared format serialized using the cereal library
    };

    /**
     * Class to hold raw, three-dimensional field data with N components, containing
     * * The actual field data as shared pointer to vector
     * * An array specifying the number of bins in each dimension
     * * An array containing the physical extent of the field in each dimension, as specified in the file
     */
    template <typename T = double> class FieldData {
    public:
        /**
         * @brief Default constructor to create an empty field data object
         */
        FieldData() = default;

        /**
         * @brief Constructor for field data
         * @param header     Human readable header string to identify file content, program version used for generation etc.
         * @param dimensions Number of bins of the field in each coordinate
         * @param size       Physical extent of the field in each dimension, given in internal units
         * @param data       Shared pointer to the flat field data
         */
        FieldData(std::string header,
                  std::array<size_t, 3> dimensions,
                  std::array<T, 3> size,
                  std::shared_ptr<std::vector<T>> data)
            : header_(std::move(header)), dimensions_(dimensions), size_(size), data_(std::move(data)){};

        /**
         * @brief Function to obtain the header (human readbale content description) of the field data
         * @return header string
         */
        std::string getHeader() const { return header_; }

        /**
         * @brief Member to get the dimensions of the field as number of bins in x, y, z
         * @return array with the number of bins in x, y and z
         */
        std::array<size_t, 3> getDimensions() const { return dimensions_; }

        /**
         * @brief Member to get the physical extent of the field in each dimension as parsed from the input in internal units
         * @return array with physical size in x, y and z
         */
        std::array<T, 3> getSize() const { return size_; }

        /**
         * @brief Member to access the actual field data
         * @return shared pointer to the flat vector of field data
         */
        std::shared_ptr<std::vector<T>> getData() const { return data_; }

        /**
         * @brief get the dimensionality of the configured field in the x-y plane, e.g whether it is defined in 1D, 2D or 3D.
         * @return Dimensionality of the field
         */
        size_t getDimensionality() const {
            size_t dim = 3;
            dim -= (dimensions_[0] == 1 ? 1u : 0);
            dim -= (dimensions_[1] == 1 ? 1u : 0);
            return dim;
        }

    private:
        std::string header_;
        std::array<size_t, 3> dimensions_{};
        std::array<T, 3> size_{};
        std::shared_ptr<std::vector<T>> data_;

        friend class cereal::access;

        // Versioned serialization function:
        template <class Archive> void serialize(Archive& archive, std::uint32_t const version) {
            // For now, we only know one version of this file type:
            if(version != 1) {
                throw std::runtime_error("unknown format version " + std::to_string(version));
            }

            // (De-) Serialize the data:
            archive(header_);
            archive(dimensions_);
            archive(size_);
            archive(data_);
        }
    };
} // namespace allpix

// Enable versioning for the FieldData class template
namespace cereal::detail {
    template <class T> struct Version<allpix::FieldData<T>> {
        static const std::uint32_t version;
        static std::uint32_t registerVersion() {
            ::cereal::detail::StaticObject<Versions>::getInstance().mapping.emplace(
                std::type_index(typeid(allpix::FieldData<T>)).hash_code(), APF_MIME_TYPE_VERSION);
            return 3;
        }
        static void unused() { (void)version; } // NOLINT
    }; /* end Version */
    template <class T>
    const std::uint32_t Version<allpix::FieldData<T>>::version = Version<allpix::FieldData<T>>::registerVersion();
} // namespace cereal::detail

namespace allpix {

    /**
     * @brief Class to parse Allpix Squared field data from files
     *
     * This class can be used to deserialize and parse FieldData objects from files of different format. The FieldData
     * objects read from file are cached, and a cache hit will be returned when trying to re-read a file with the same
     * canonical path.
     */
    template <typename T = double> class FieldParser {
    public:
        /**
         * Construct a FieldParser
         * @param quantity Quantity of individual field points, vector (three values per point) or scalar (one value per
         * point)
         */
        explicit FieldParser(const FieldQuantity quantity)
            : N_(static_cast<std::underlying_type<FieldQuantity>::type>(quantity)){};
        ~FieldParser() = default;

        /**
         * @brief Parse a file and retrieve the field data.
         * @param file_name  File name (as canonical path) of the input file to be parsed
         * @param units      Optional units to convert the field from after reading from file. Only used by some formats.
         * @return           Field data object read from file or internal cache
         *
         * @throws std::runtime_error if the file format is unknown or invalid field dimensions are detected
         * @throws std::filesystem::filesystem_error if the provided path does not exist
         *
         * The type of the field data file to be read is deducted automatically from the file content
         */
        FieldData<T> getByFileName(const std::filesystem::path& file_name, const std::string& units = std::string()) {

            auto path = std::filesystem::canonical(file_name);

            // Search in cache
            auto iter = field_map_.find(path);
            if(iter != field_map_.end()) {
                LOG(INFO) << "Using cached field data";
                return iter->second;
            }

            // Deduce the file format
            auto file_type = guess_file_type(path);
            LOG(DEBUG) << "Assuming file type \"" << (file_type == FileType::APF ? "APF" : "INIT") << "\"";

            FieldData<T> field_data;
            switch(file_type) {
            case FileType::INIT:
                if(units.empty()) {
                    LOG(WARNING) << "No field units provided, interpreting field data in internal units, this might lead to "
                                    "unexpected results.";
                }
                field_data = parse_init_file(path, units);
                break;
            case FileType::APF:
                if(!units.empty()) {
                    LOG(DEBUG) << "Units will be ignored, APF file content is interpreted in internal units.";
                }
                field_data = parse_apf_file(path);
                break;
            default:
                throw std::runtime_error("unknown file format");
            }

            // Store the parsed field data for further reference:
            field_map_[path] = field_data;
            return field_data;
        }

    private:
        /**
         * @brief Check if the file is a binary file
         * @param path The path to the file to be checked check
         * @return True if the file contains null bytes, false otherwise
         *
         * This helper function checks the first 256 characters of a file for the occurrence of a nullbyte.
         * For binary files it is very unlikely not to have at least one. This approach is also used e.g. by diff
         */
        bool file_is_binary(const std::filesystem::path& path) const {
            std::ifstream file(path);
            for(size_t i = 0; i < 256; i++) {
                if(file.get() == '\0') {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Function to guess the type of a field data file
         * @param path Path to the file to be tested
         * @return Type of the file
         *
         * This function checks if the file contains binary data to interpret it as APF formator INIT format otherwise.
         */
        FileType guess_file_type(const std::filesystem::path& path) const {
            return (file_is_binary(path) ? FileType::APF : FileType::INIT);
        }

        /**
         * @brief Function to deserialize FieldData from an APF file, using the cereal library. This does not convert any
         * units, i.e. all values stored in APF files are given framework-internal base units. This includes the field data
         * itself as well as the field size.
         * @param file_name  File name (as canonical path) of the input file to be parsed
         */
        FieldData<T> parse_apf_file(const std::filesystem::path& file_name) {
            std::ifstream file(file_name, std::ios::binary);
            FieldData<T> field_data;

            // Parse the file with cereal, scope ensures flushing:
            try {
                cereal::PortableBinaryInputArchive archive(file);
                archive(field_data);
            } catch(cereal::Exception& e) {
                throw std::runtime_error(e.what());
            }

            // Check that we have the right number of vector entries
            auto dimensions = field_data.getDimensions();
            if(field_data.getData()->size() != dimensions[0] * dimensions[1] * dimensions[2] * N_) {
                throw std::runtime_error("invalid data");
            }

            return field_data;
        }

        /**
         * @brief Helper function to compare potential units defined in the INIT file against the ones provided:
         * @param file_units Unit string read from the file
         * @param units      Unit string provided via the parser interface
         */
        void check_unit_match(const std::string& file_units, const std::string& units) {
            // If we read "##SEED#" or a number, the file is provided in the original format and we ignore it.
            if(file_units == "##SEED##" || std::all_of(file_units.begin(), file_units.end(), ::isdigit)) {
                LOG(DEBUG) << "INIT file does not contain unit information. Header states \"" << file_units << "\"";
            } else if(file_units == "internal") { // File reports internal units
                // Parser requests unit conversion:
                if(!units.empty()) {
                    LOG(ERROR) << "Requesting to interpret INIT field as units \"" << units
                               << "\" while file header states internal units";
                } else {
                    LOG(DEBUG) << "INIT file states internal units, so does the parser";
                }
            } else if(Units::get(file_units) != Units::get(units)) { // File reports units
                LOG(ERROR) << "Requesting to interpret INIT field as units \"" << units << "\" while file header states \""
                           << file_units << "\"";
            }
        }

        /**
         * @brief Function to read FieldData from INIT-formatted ASCII files. Values are interpreted in the units provided by
         * the argument and converted to the framework-internal base units. The size of the field given in the file is always
         * interpreted as micrometers.
         * @param file_name  File name (as canonical path) of the input file to be parsed
         * @param units      Units to convert the values of the field data from
         */
        FieldData<T> parse_init_file(const std::filesystem::path& file_name, const std::string& units) {
            // Load file
            std::ifstream file(file_name);
            std::string header;
            std::getline(file, header);
            LOG(TRACE) << "Header of file " << file_name << " is " << std::endl << header;

            // Read the header
            std::string tmp;
            // WARNING the usage of this field as storage for the field units differs from the original INIT format!
            file >> tmp;
            check_unit_match(allpix::trim(tmp), units);
            file >> tmp;               // ignore cluster length
            file >> tmp >> tmp >> tmp; // ignore the incident pion direction
            file >> tmp >> tmp >> tmp; // ignore the magnetic field (specify separately)
            double thickness = NAN, xpixsz = NAN, ypixsz = NAN;
            file >> thickness >> xpixsz >> ypixsz;
            thickness = Units::get(thickness, "um");
            xpixsz = Units::get(xpixsz, "um");
            ypixsz = Units::get(ypixsz, "um");
            file >> tmp >> tmp >> tmp >> tmp; // ignore temperature, flux, rhe (?) and new_drde (?)
            size_t xsize = 0, ysize = 0, zsize = 0;
            file >> xsize >> ysize >> zsize;
            file >> tmp;

            if(file.fail()) {
                throw std::runtime_error("invalid data or unexpected end of file");
            }
            auto field = std::make_shared<std::vector<double>>();
            auto vertices = xsize * ysize * zsize;
            field->resize(vertices * N_);

            // Loop through all the field data
            for(size_t i = 0; i < vertices; ++i) {
                if(vertices >= 100 && i % (vertices / 100) == 0) {
                    LOG_PROGRESS(INFO, "read_init") << "Reading field data: " << (100 * i / vertices) << "%";
                }

                if(file.eof()) {
                    throw std::runtime_error("unexpected end of file");
                }

                // Get index of field
                size_t xind = 0, yind = 0, zind = 0;
                file >> xind >> yind >> zind;

                if(file.fail() || xind > xsize || yind > ysize || zind > zsize) {
                    throw std::runtime_error("invalid data");
                }
                xind--;
                yind--;
                zind--;

                // Loop through components of field
                for(size_t j = 0; j < N_; ++j) {
                    double input = NAN;
                    file >> input;

                    // Set the field at a position
                    (*field)[xind * ysize * zsize * N_ + yind * zsize * N_ + zind * N_ + j] = Units::get(input, units);
                }
            }
            LOG_PROGRESS(INFO, "read_init") << "Reading field data: finished.";

            return FieldData<T>(
                header, std::array<size_t, 3>{{xsize, ysize, zsize}}, std::array<T, 3>{{xpixsz, ypixsz, thickness}}, field);
        }

        size_t N_;
        std::map<std::filesystem::path, FieldData<T>> field_map_;
    };

    /**
     * @brief Class to write Allpix Squared field data to files
     *
     * This class can be used to serialize FieldData objects into files using different formats. Scalar as well as vector
     * fields are supported.
     */
    template <typename T = double> class FieldWriter {
    public:
        /**
         * @brief Construct a FileWriter
         * @param quantity Quantity of individual field points, vector (three values per point) or scalar (one value per
         * point)
         */
        explicit FieldWriter(const FieldQuantity quantity)
            : N_(static_cast<std::underlying_type<FieldQuantity>::type>(quantity)){};
        ~FieldWriter() = default;

        /**
         * @brief Write the field to a file
         * @param field_data Field data object to store
         * @param file_name  File name (as canonical path) of the output file to be created
         * @param file_type  Type of file (file format) to be produced
         * @param units      Optional units to convert the field into before writing. Only used by some formats.
         * @throws std::runtime_error if the file format is unknown or invalid field dimensions are detected
         * @throws std::filesystem::filesystem_error if the provided path does not exist
         */
        void writeFile(const FieldData<T>& field_data,
                       const std::filesystem::path& file_name,
                       const FileType& file_type,
                       const std::string& units = std::string()) {

            auto path = std::filesystem::weakly_canonical(file_name);

            auto dimensions = field_data.getDimensions();
            if(field_data.getData()->size() != N_ * dimensions[0] * dimensions[1] * dimensions[2]) {
                throw std::runtime_error("invalid field dimensions");
            }

            switch(file_type) {
            case FileType::INIT:
                if(units.empty()) {
                    LOG(WARNING) << "No field units provided, writing field data in internal units.";
                }
                write_init_file(field_data, path, units);
                break;
            case FileType::APF:
                if(!units.empty()) {
                    LOG(WARNING) << "Units will be ignored, APF file content is written in internal units.";
                }
                write_apf_file(field_data, path);
                break;
            default:
                throw std::runtime_error("unknown file format");
            }
        }

    private:
        /**
         * @brief Function to serialize FieldData into an APF file, using the cereal library. This does not convert any
         * units, i.e. all values stored in APF files are given framework-internal base units. This includes the field data
         * itself as well as the field size.
         * @param field_data Field data object to store
         * @param file_name  File name (as canonical path) of the output file to be created
         */
        void write_apf_file(const FieldData<T>& field_data, const std::filesystem::path& file_name) {
            std::ofstream file(file_name, std::ios::binary);

            // Write the file with cereal:
            try {
                cereal::PortableBinaryOutputArchive archive(file);
                archive(field_data);
            } catch(cereal::Exception& e) {
                throw std::runtime_error(e.what());
            }
        }

        /**
         * @brief Function to write FieldData objects out to INIT-formatted ASCII files. Values are converted from the
         * framework-internal base units in which the data is stored in FieldData into the units provided by the units
         * parameter. The size of the field is always converted to micrometers.
         * @param field_data Field data object to store
         * @param file_name  File name (as canonical path) of the output file to be created
         * @param units      Units to convert the values of the field data to.
         */
        void
        write_init_file(const FieldData<T>& field_data, const std::filesystem::path& file_name, const std::string& units) {
            std::ofstream file(file_name);

            LOG(TRACE) << "Writing INIT file \"" << file_name << "\"";

            // Write INIT file header
            file << field_data.getHeader() << std::endl;                                 // Header line
            file << (!units.empty() ? units : "internal") << " ##EVENTS##" << std::endl; // Use placeholder for units
            file << "##TURN## ##TILT## 1.0" << std::endl;                                // Unused
            file << "0.0 0.0 0.0" << std::endl;                                          // Magnetic field (unused)

            auto size = field_data.getSize();
            file << Units::convert(size[2], "um") << " " << Units::convert(size[0], "um") << " "
                 << Units::convert(size[1], "um") << " "; // Field size: (z, x, y)
            file << "0.0 0.0 0.0 0.0 ";                   // Unused

            auto dimensions = field_data.getDimensions();
            file << dimensions[0] << " " << dimensions[1] << " " << dimensions[2] << " "; // Field grid dimensions (x, y, z)
            file << "0.0" << std::endl;                                                   // Unused

            // Write the data block:
            auto data = field_data.getData();
            auto max_points = data->size() / N_;

            for(size_t xind = 0; xind < dimensions[0]; ++xind) {
                for(size_t yind = 0; yind < dimensions[1]; ++yind) {
                    for(size_t zind = 0; zind < dimensions[2]; ++zind) {
                        // Write field point index
                        file << xind + 1 << " " << yind + 1 << " " << zind + 1;

                        // Vector or scalar field:
                        for(size_t j = 0; j < N_; j++) {
                            file << " "
                                 << Units::convert(data->at(xind * dimensions[1] * dimensions[2] * N_ +
                                                            yind * dimensions[2] * N_ + zind * N_ + j),
                                                   units);
                        }
                        // End this line
                        file << std::endl;
                    }

                    auto curr_point = xind * dimensions[1] * dimensions[2] + yind * dimensions[2];
                    LOG_PROGRESS(INFO, "write_init") << "Writing field data: " << (100 * curr_point / max_points) << "%";
                }
            }
            LOG_PROGRESS(INFO, "write_init") << "Writing field data: finished.";
        }

        size_t N_;
    };
} // namespace allpix

#endif /* ALLPIX_FIELD_PARSER_H */
