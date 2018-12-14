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

namespace allpix {

    /**
     * Three-dimensional field data with N components, containing
     * * The actual field data as shared pointer to vector
     * * An array specifying the number of bins in each dimension
     * * An array containing the physical extent of the field in each dimension, as specified in the file
     */
    template <typename T = double>
    using FieldData = std::tuple<std::shared_ptr<std::vector<T>>, std::array<size_t, 3>, std::array<T, 3>>;

    template <typename T = double, int N = 3> class FieldParser {
    public:
        FieldParser(const std::string units) : units_(std::move(units)){};
        ~FieldParser() = default;

        /**
         * @brief Get the field from a file name, caching the result
         */
        FieldData<T> get_by_file_name(const std::string& file_name) {
            // Search in cache (NOTE: the path reached here is always a canonical name)
            auto iter = field_map_.find(file_name);
            if(iter != field_map_.end()) {
                LOG(INFO) << "Using cached field data";
                return iter->second;
            }

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

            FieldData<T> field_data = std::make_tuple(
                field, std::array<size_t, 3>{{xsize, ysize, zsize}}, std::array<double, 3>{{xpixsz, ypixsz, thickness}});

            // Store the parsed field data for further reference:
            field_map_[file_name] = field_data;
            return field_data;
        }

    private:
        std::string units_;
        std::map<std::string, FieldData<T>> field_map_;
    };
} // namespace allpix

#endif /* ALLPIX_FIELD_PARSER_H */
