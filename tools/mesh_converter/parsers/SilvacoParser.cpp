/**
 * @file
 *
 * @copyright Copyright (c) 2022-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SilvacoParser.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/utils/log.h"
#include "core/utils/text.h"

using namespace mesh_converter;

MeshMap SilvacoParser::read_meshes(const std::string& file_name) {

    std::ifstream file(file_name);
    if(!file) {
        throw std::runtime_error("file cannot be accessed");
    }

    // Get the total number of lines to parse and reset file to the start:
    auto num_lines = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
    file.clear();
    file.seekg(0, std::ios::beg);
    LOG(DEBUG) << "Grid file contains " << num_lines << " lines to parse";

    std::vector<Point> vertices;

    long unsigned int dimension = 1;
    long unsigned int columns_count = 0;
    long long num_lines_parsed = 0;
    while(!file.eof()) {
        std::string line;
        std::getline(file, line);

        // Log the parsing progress:
        if(num_lines > 0 && num_lines_parsed % 1000 == 0) {
            LOG_PROGRESS(STATUS, "gridlines") << "Parsing grid file: " << (100 * num_lines_parsed / num_lines) << "%";
        }
        num_lines_parsed++;

        line = allpix::trim(line);
        if(line.empty()) {
            continue;
        }

        // Determining number of columns by counting fields in first line
        if(num_lines_parsed == 1) {
            std::istringstream iss;
            iss.str(line);
            double val = NAN;
            while(iss >> val) {
                columns_count++;
            }
            dimension = columns_count;
            iss.clear();
        }

        // Handle data
        std::stringstream sstr(line);
        // Read vertex points
        if(dimension == 3) {
            double x = 0;
            double y = 0;
            double z = 0;
            while(sstr >> x >> y >> z) {
                vertices.emplace_back(x, y, z);
            }
        }
        if(dimension == 2) {
            double y = 0;
            double z = 0;
            while(sstr >> y >> z) {
                vertices.emplace_back(y, z);
            }
        }
    }
    LOG_PROGRESS(STATUS, "gridlines") << "Parsing grid file: done.";

    return {{"Silicon", vertices}};
}

FieldMap SilvacoParser::read_fields(const std::string& file_name, const std::string& observable) {
    std::ifstream file(file_name);
    if(!file) {
        throw std::runtime_error("file cannot be accessed");
    }

    // Get the total number of lines to parse and reset file to the start:
    auto num_lines = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
    file.clear();
    file.seekg(0, std::ios::beg);
    LOG(DEBUG) << "Field data file contains " << num_lines << " lines to parse";

    // std::map<std::string, std::vector<Point>> region_electric_field_map;
    std::map<std::string, std::map<std::string, std::vector<Point>>> region_electric_field_map;
    std::vector<double> region_electric_field_num;

    std::string const region = "Silicon";
    long unsigned int dimension = 1;
    long long num_lines_parsed = 0;
    long unsigned int columns_count = 0;
    while(!file.eof()) {
        std::string line;
        std::getline(file, line);
        line = allpix::trim(line);

        // Log the parsing progress:
        if(num_lines > 0 && num_lines_parsed % 1000 == 0) {
            LOG_PROGRESS(STATUS, "fieldlines") << "Parsing field data file: " << (100 * num_lines_parsed / num_lines) << "%";
        }
        num_lines_parsed++;

        if(line.empty()) {
            continue;
        }

        // Determining number of columns by counting fields in first line
        if(num_lines_parsed == 1) {
            std::istringstream iss;
            iss.str(line);
            double val = NAN;
            while(iss >> val) {
                columns_count++;
            }
            dimension = columns_count;

            iss.clear();
        }

        // Handle data
        std::stringstream sstr(line);
        double num = NAN;
        while(sstr >> num) {
            region_electric_field_num.push_back(num);
        }

        // Determining data type from dimensions
        // dimension == 1 -> Scalar potential
        // dimension == 2 -> 2D electric field
        // dimension == 3 -> 3D electric field
        if(dimension == 1) {
            for(size_t i = 0; i < region_electric_field_num.size(); i += 1) {
                auto x = region_electric_field_num[i];
                region_electric_field_map[region][observable].emplace_back(x, 0, 0);
            }

            region_electric_field_num.clear();
        } else if(dimension == 3) {
            for(size_t i = 0; i < region_electric_field_num.size(); i += 3) {
                auto x = region_electric_field_num[i];
                auto y = region_electric_field_num[i + 1];
                auto z = region_electric_field_num[i + 2];
                region_electric_field_map[region][observable].emplace_back(x, y, z);
            }
        } else if(dimension == 2) {
            for(size_t i = 0; i < region_electric_field_num.size(); i += 2) {
                auto x = region_electric_field_num[i];
                auto y = region_electric_field_num[i + 1];
                region_electric_field_map[region][observable].emplace_back(0, x, y);
            }
        } else {
            throw std::runtime_error("incorrect dimension of observable");
        }

        region_electric_field_num.clear();
    }

    LOG_PROGRESS(STATUS, "fieldlines") << "Parsing field data file: done.";

    return region_electric_field_map;
}
