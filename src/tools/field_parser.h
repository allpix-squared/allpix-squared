/**
 * @file
 * @brief Utility to parse INIT-format field files
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_FIELD_PARSER_H
#define ALLPIX_FIELD_PARSER_H

#include <fstream>
#include <iostream>
#include <map>

#include "core/utils/unit.h"

#include <cereal/archives/portable_binary.hpp>

#include <cereal/types/array.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

namespace allpix {

    /**
     * @brief Type of file formats
     */
    enum class FileType {
        UNKNOWN = 0, ///< Unknown file format
        INIT,        ///< Leagcy file format, values stored in plain-text ASCII
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
        FieldData() = default;
        FieldData(std::string header,
                  std::array<size_t, 3> dimensions,
                  std::array<T, 3> size,
                  std::shared_ptr<std::vector<T>> data)
            : header_(header), dimensions_(dimensions), size_(size), data_(data){};

        std::string getHeader() const { return header_; }
        std::array<size_t, 3> getDimensions() const { return dimensions_; }
        std::array<T, 3> getSize() const { return size_; }
        std::shared_ptr<std::vector<T>> getData() const { return data_; }

    private:
        std::string header_;
        std::array<size_t, 3> dimensions_{};
        std::array<T, 3> size_{};
        std::shared_ptr<std::vector<T>> data_;

        friend class cereal::access;

        template <class Archive> void serialize(Archive& archive) {
            archive(header_);
            archive(dimensions_);
            archive(size_);
            archive(data_);
        }
    };

    template <typename T = double, int N = 3> class FieldParser {
    public:
        FieldParser(const std::string units) : units_(std::move(units)){};
        ~FieldParser() = default;

        /**
         * @brief Get the field from a file name, caching the result
         */
        FieldData<T> get_by_file_name(const std::string& file_name, const FileType& file_type) {
            // Search in cache (NOTE: the path reached here is always a canonical name)
            auto iter = field_map_.find(file_name);
            if(iter != field_map_.end()) {
                LOG(INFO) << "Using cached field data";
                return iter->second;
            }

            switch(file_type) {
            case FileType::INIT:
                return parse_init_file(file_name);
            case FileType::APF:
                return parse_apf_file(file_name);
            default:
                throw std::runtime_error("unknown file format");
            }
        }

    private:
        FieldData<T> parse_apf_file(const std::string& file_name) {
            std::ifstream file(file_name, std::ios::binary);
            FieldData<double> field_data;

            // Parse the file with cereal, add manual scope to ensure flushing:
            {
                cereal::PortableBinaryInputArchive archive(file);
                archive(field_data);
            }

            // Check that we have the right number of vector entries
            auto dimensions = field_data.getDimensions();
            if(field_data.getData()->size() != dimensions[0] * dimensions[1] * dimensions[2] * N) {
                throw std::runtime_error("invalid data");
            }

            return field_data;
        }

        FieldData<T> parse_init_file(const std::string& file_name) {
            // Load file
            std::ifstream file(file_name);
            std::string header;
            std::getline(file, header);
            LOG(TRACE) << "Header of file " << file_name << " is " << std::endl << header;

            // Read the header
            std::string tmp;
            file >> tmp >> tmp;        // ignore the init seed and cluster length
            file >> tmp >> tmp >> tmp; // ignore the incident pion direction
            file >> tmp >> tmp >> tmp; // ignore the magnetic field (specify separately)
            double thickness, xpixsz, ypixsz;
            file >> thickness >> xpixsz >> ypixsz;
            thickness = Units::get(thickness, "um");
            xpixsz = Units::get(xpixsz, "um");
            ypixsz = Units::get(ypixsz, "um");
            file >> tmp >> tmp >> tmp >> tmp; // ignore temperature, flux, rhe (?) and new_drde (?)
            size_t xsize, ysize, zsize;
            file >> xsize >> ysize >> zsize;
            file >> tmp;

            if(file.fail()) {
                throw std::runtime_error("invalid data or unexpected end of file");
            }
            auto field = std::make_shared<std::vector<double>>();
            auto vertices = xsize * ysize * zsize;
            field->resize(vertices * N);

            // Loop through all the field data
            for(size_t i = 0; i < vertices; ++i) {
                if(i % (vertices / 100) == 0) {
                    LOG_PROGRESS(INFO, "read_init") << "Reading field data: " << (100 * i / vertices) << "%";
                }

                if(file.eof()) {
                    throw std::runtime_error("unexpected end of file");
                }

                // Get index of field
                size_t xind, yind, zind;
                file >> xind >> yind >> zind;

                if(file.fail() || xind > xsize || yind > ysize || zind > zsize) {
                    throw std::runtime_error("invalid data");
                }
                xind--;
                yind--;
                zind--;

                // Loop through components of field
                for(size_t j = 0; j < N; ++j) {
                    double input;
                    file >> input;

                    // Set the field at a position
                    (*field)[xind * ysize * zsize * N + yind * zsize * N + zind * N + j] = Units::get(input, units_);
                }
            }
            LOG_PROGRESS(INFO, "read_init") << "Reading field data: finished.";

            FieldData<T> field_data(
                header, std::array<size_t, 3>{{xsize, ysize, zsize}}, std::array<T, 3>{{xpixsz, ypixsz, thickness}}, field);

            // Store the parsed field data for further reference:
            field_map_[file_name] = field_data;
            return field_data;
        }

        std::string units_;
        std::map<std::string, FieldData<T>> field_map_;
    };
} // namespace allpix

#endif /* ALLPIX_FIELD_PARSER_H */
