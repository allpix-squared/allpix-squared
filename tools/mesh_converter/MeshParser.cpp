/**
 * @file
 *
 * @copyright Copyright (c) 2020-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <iomanip>

#include "MeshParser.hpp"

using namespace mesh_converter;

std::shared_ptr<MeshParser> MeshParser::factory(const allpix::Configuration& config) {
    auto parser = config.get<std::string>("parser", "df-ise");
    std::transform(parser.begin(), parser.end(), parser.begin(), ::tolower);
    LOG(DEBUG) << "Parser \"" << parser << "\"";
    if(parser == "df-ise" || parser == "dfise") {
        return std::make_shared<DFISEParser>();
    } else if(parser == "silvaco") {
        return std::make_shared<SilvacoParser>();
    } else {
        throw allpix::InvalidValueError(config, "parser", "Unknown parser type");
    }
}

std::vector<Point> MeshParser::getMesh(const std::string& file, const std::vector<std::string>& regions) {

    // Populate mesh map once:
    if(mesh_map_[file].empty()) {
        LOG(STATUS) << "Reading mesh grid from file \"" << file << "\"";
        mesh_map_[file] = read_meshes(file);
        LOG(INFO) << "Grid sizes for all regions:";
        for(auto& reg : mesh_map_[file]) {
            LOG(INFO) << "\t" << std::left << std::setw(25) << reg.first << " " << reg.second.size();
        }
    } else {
        LOG(STATUS) << "Using cached mesh grid from file \"" << file << "\"";
    }

    // Append all grid regions to the mesh:
    std::vector<Point> points;
    for(const auto& region : regions) {
        if(mesh_map_[file].find(region) != mesh_map_[file].end()) {
            points.insert(points.end(), mesh_map_[file][region].begin(), mesh_map_[file][region].end());
        } else {
            throw std::runtime_error("Region \"" + region + "\" not found in mesh file");
        }
    }

    if(points.empty()) {
        throw std::runtime_error("Empty grid");
    }
    LOG(DEBUG) << "Grid with " << points.size() << " points";

    return points;
}

std::vector<Point>
MeshParser::getField(const std::string& file, const std::string& observable, const std::vector<std::string>& regions) {

    // Populate field map once:
    if(field_map_[file].empty()) {
        LOG(STATUS) << "Reading field from file \"" << file << "\"";
        field_map_[file] = read_fields(file, observable);
        LOG(INFO) << "Field sizes for all regions and observables:";
        for(auto& reg : field_map_[file]) {
            LOG(INFO) << " " << reg.first << ":";
            for(auto& fld : reg.second) {
                LOG(INFO) << "\t" << std::left << std::setw(25) << fld.first << " " << fld.second.size();
            }
        }
    } else {
        LOG(STATUS) << "Using cached field from file \"" << file << "\"";
    }

    // Append all field regions to the field vector:
    std::vector<Point> field;
    for(const auto& region : regions) {
        if(field_map_[file].find(region) != field_map_[file].end() &&
           field_map_[file][region].find(observable) != field_map_[file][region].end()) {
            LOG(DEBUG) << "Region \"" << region << "\"";
            field.insert(
                field.end(), field_map_[file][region][observable].begin(), field_map_[file][region][observable].end());
        } else {
            throw std::runtime_error("No matching region with observable \"" + observable + "\" found in field file");
        }
    }

    if(field.empty()) {
        throw std::runtime_error("Empty observable data");
    }
    LOG(DEBUG) << "Field with " << field.size() << " points";

    return field;
}
