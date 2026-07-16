/**
 * @file
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SilvacoSSFParser.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>

#include "core/utils/log.h"
#include "core/utils/text.h"

using namespace mesh_converter;

namespace {
    // Silvaco quantity codes for the observables supported by the mesh converter
    const std::map<std::string, std::vector<int>>& observable_codes() {
        static const std::map<std::string, std::vector<int>> codes = {
            // E Field Z is code 122 in ATLAS output but 613 in Victory Device output:
            {"ElectricField", {120, 121, 122, 613}},
            {"ElectrostaticPotential", {100}},
            {"DopingConcentration", {115}},
            {"NetDoping", {115}},
            {"DonorConcentration", {71}},
            {"AcceptorConcentration", {72}},
        };
        return codes;
    }

    constexpr int CODE_POTENTIAL = 100;
} // namespace

std::string SilvacoSSFParser::resolve_file(const std::string& file_name) {
    // The converter appends .grd/.dat to the file prefix; look for the raw Silvaco file instead
    auto base = file_name;
    auto dot = base.find_last_of('.');
    if(dot != std::string::npos) {
        base = base.substr(0, dot);
    }
    for(const auto& candidate : {base + ".sta", base + ".str", file_name}) {
        const std::ifstream file(candidate);
        if(file) {
            return candidate;
        }
    }
    throw std::runtime_error("no Silvaco structure file found for prefix \"" + base + "\" (tried .sta, .str)");
}

void SilvacoSSFParser::parse_file(const std::string& file_name,
                                  std::map<int, std::array<double, 3>>& coords,
                                  std::vector<int>& codes,
                                  std::map<int, std::vector<double>>& node_values,
                                  bool& two_dimensional) {
    std::ifstream file(file_name);
    if(!file) {
        throw std::runtime_error("file cannot be accessed");
    }

    // Get the total number of lines to parse and reset file to the start:
    auto num_lines = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
    file.clear();
    file.seekg(0, std::ios::beg);
    LOG(DEBUG) << "Structure file contains " << num_lines << " lines to parse";

    std::map<int, double> best_key;
    size_t potential_column = 0;
    bool has_potential = false;

    long long num_lines_parsed = 0;
    std::string line;
    while(std::getline(file, line)) {
        if(num_lines > 0 && num_lines_parsed % 10000 == 0) {
            LOG_PROGRESS(STATUS, "ssflines") << "Parsing structure file: " << (100 * num_lines_parsed / num_lines) << "%";
        }
        num_lines_parsed++;

        if(line.size() < 2 || line[1] != ' ') {
            continue;
        }

        if(line[0] == 'c') {
            // Coordinate record: c <id> <x> <y> [<z>]
            std::istringstream sstr(line.substr(1));
            int id = 0;
            double x = 0;
            double y = 0;
            double z = 0;
            if(sstr >> id >> x >> y) {
                sstr >> z;
                coords[id] = {x, y, z};
            }
        } else if(line[0] == 's') {
            // Solution specification: s <n> <code_1> ... <code_n>
            std::istringstream sstr(line.substr(1));
            size_t n = 0;
            sstr >> n;
            std::vector<int> new_codes;
            int code = 0;
            while(new_codes.size() < n && sstr >> code) {
                new_codes.push_back(code);
            }
            if(!codes.empty() && new_codes != codes) {
                throw std::runtime_error("multiple conflicting solution specifications found in \"" + file_name + "\"");
            }
            codes = new_codes;
            auto pot = std::ranges::find(codes, CODE_POTENTIAL);
            has_potential = (pot != codes.end());
            potential_column = static_cast<size_t>(pot - codes.begin());
        } else if(line[0] == 'n' && !codes.empty()) {
            // Nodal values: n <node_id> <region> <value_1> ... <value_n>
            std::istringstream sstr(line.substr(1));
            int node_id = 0;
            int region = 0;
            if(!(sstr >> node_id >> region)) {
                continue;
            }
            std::vector<double> values;
            values.reserve(codes.size());
            double value = 0;
            while(values.size() < codes.size() && sstr >> value) {
                values.push_back(value);
            }
            if(values.size() < codes.size()) {
                continue;
            }
            // Coordinate ids are 1-based while node ids are 0-based:
            const int coord_id = node_id + 1;
            // Interface nodes are duplicated between regions; keep the semiconductor side which carries
            // the largest absolute potential. Without a potential column the first record is kept.
            const double key = has_potential ? std::fabs(values[potential_column]) : 0.;
            auto iter = best_key.find(coord_id);
            if(iter == best_key.end() || key > iter->second) {
                best_key[coord_id] = key;
                node_values[coord_id] = std::move(values);
            }
        }
    }
    LOG_PROGRESS(STATUS, "ssflines") << "Parsing structure file: done.";

    if(codes.empty()) {
        throw std::runtime_error("no solution specification found in \"" + file_name + "\"");
    }

    // The mesh is two-dimensional if all z coordinates are zero:
    two_dimensional = std::ranges::all_of(coords, [](const auto& c) noexcept { return c.second[2] == 0.; });
}

MeshMap SilvacoSSFParser::read_meshes(const std::string& file_name) {
    std::map<int, std::array<double, 3>> coords;
    std::vector<int> codes;
    std::map<int, std::vector<double>> node_values;
    bool two_dimensional = false;
    parse_file(resolve_file(file_name), coords, codes, node_values, two_dimensional);

    std::vector<Point> vertices;
    vertices.reserve(node_values.size());
    for(const auto& node : node_values) {
        const auto& coord = coords.at(node.first);
        if(two_dimensional) {
            vertices.emplace_back(coord[0], coord[1]);
        } else {
            vertices.emplace_back(coord[0], coord[1], coord[2]);
        }
    }
    LOG(INFO) << "Mesh with " << vertices.size() << " unique nodes, " << (two_dimensional ? "2D" : "3D");

    return {{"Silicon", vertices}};
}

FieldMap SilvacoSSFParser::read_fields(const std::string& file_name, const std::string& observable) {
    std::map<int, std::array<double, 3>> coords;
    std::vector<int> codes;
    std::map<int, std::vector<double>> node_values;
    bool two_dimensional = false;
    parse_file(resolve_file(file_name), coords, codes, node_values, two_dimensional);

    const auto& codes_map = observable_codes();
    const auto obs_it = codes_map.find(observable);
    if(obs_it == codes_map.end()) {
        std::string known;
        for(const auto& entry : codes_map) {
            known += " " + entry.first;
        }
        throw std::runtime_error("unknown observable \"" + observable + "\", supported:" + known);
    }

    // Map the requested quantity codes to value columns:
    std::vector<size_t> columns;
    for(const auto code : obs_it->second) {
        auto iter = std::ranges::find(codes, code);
        if(iter != codes.end()) {
            columns.push_back(static_cast<size_t>(iter - codes.begin()));
        }
    }

    const bool vector_field = (obs_it->second.size() > 1);
    if(columns.empty() || (vector_field && columns.size() < 2)) {
        throw std::runtime_error("observable \"" + observable + "\" not present in file \"" + file_name + "\"");
    }
    if(vector_field && !two_dimensional && columns.size() < 3) {
        throw std::runtime_error("three-dimensional mesh but no z component found for observable \"" + observable + "\"");
    }

    std::vector<Point> field;
    field.reserve(node_values.size());
    for(const auto& node : node_values) {
        const auto& values = node.second;
        if(!vector_field) {
            field.emplace_back(values[columns[0]], 0, 0);
        } else if(two_dimensional) {
            field.emplace_back(0, values[columns[0]], values[columns[1]]);
        } else {
            field.emplace_back(values[columns[0]], values[columns[1]], values[columns[2]]);
        }
    }
    LOG(INFO) << "Field \"" << observable << "\" with " << field.size() << " values";

    return {{"Silicon", {{observable, field}}}};
}
