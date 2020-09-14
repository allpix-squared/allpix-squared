#include "DFISEParser.hpp"

#include <cassert>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include "TFile.h"
#include "TTree.h"

#include "core/utils/log.h"
#include "core/utils/text.h"

using namespace mesh_converter;

std::map<std::string, std::vector<Point>> mesh_converter::read_grid(const std::string& file_name, bool mesh_tree) {
    std::ifstream file(file_name);
    if(!file) {
        throw std::runtime_error("file cannot be accessed");
    }

    // Get the total number of lines to parse and reset file to the start:
    auto num_lines = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
    file.clear();
    file.seekg(0, std::ios::beg);
    LOG(DEBUG) << "Grid file contains " << num_lines << " lines to parse";

    DFSection main_section = DFSection::HEADER;
    DFSection sub_section = DFSection::NONE;

    std::vector<Point> vertices;
    std::vector<std::pair<long unsigned int, long unsigned int>> edges;
    std::vector<std::vector<long unsigned int>> faces;
    std::vector<std::vector<long unsigned int>> elements;

    std::map<std::string, std::vector<long unsigned int>> regions_vertices;

    Point point(-1.0, -1.0, -1.0);
    auto tree = new TTree("mesh_points", "Mesh points");
    tree->Branch("x", &point.x, "x/D");
    tree->Branch("y", &point.y, "y/D");
    tree->Branch("z", &point.z, "z/D");

    std::string region;
    long unsigned int dimension = 1;
    long unsigned int data_count = 0;
    bool in_data_block = false;
    long long num_lines_parsed = 0;
    while(!file.eof()) {
        std::string line;
        std::getline(file, line);

        // Log the parsing progress:
        if(num_lines_parsed % 1000 == 0 && num_lines > 0) {
            LOG_PROGRESS(INFO, "gridlines") << "Parsing grid file: " << (100 * num_lines_parsed / num_lines) << "%";
        }
        num_lines_parsed++;

        line = allpix::trim(line);
        if(line.empty()) {
            continue;
        }

        // Check if line with begin of section
        if(line.find('{') != std::string::npos) {
            // Search for new simple headers
            auto base_regex = std::regex("([a-zA-Z]+) \\{");
            std::smatch base_match;
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto header_string = base_match[1].str();

                if(header_string == "Info") {
                    main_section = DFSection::INFO;
                } else if(header_string == "Data") {
                    in_data_block = true;
                } else {
                    if(main_section != DFSection::NONE) {
                        sub_section = DFSection::IGNORED;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                }
            }

            // Search for headers with data
            base_regex = std::regex("([a-zA-Z]+) \\((\\S+)\\) \\{");
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto header_string = base_match[1].str();
                auto header_data = base_match[2].str();

                if(header_string == "Region") {
                    main_section = DFSection::REGION;
                    region = header_data.substr(1, header_data.size() - 2);
                } else if(header_string == "Vertices") {
                    main_section = DFSection::VERTICES;
                    data_count = std::stoul(header_data);
                } else if(header_string == "Edges") {
                    main_section = DFSection::EDGES;
                    data_count = std::stoul(header_data);
                } else if(header_string == "Faces") {
                    main_section = DFSection::FACES;
                    data_count = std::stoul(header_data);
                } else if(header_string == "Elements") {
                    if(main_section == DFSection::REGION) {
                        sub_section = DFSection::ELEMENTS;
                    } else {
                        main_section = DFSection::ELEMENTS;
                    }
                    data_count = std::stoul(header_data);
                } else {
                    if(main_section != DFSection::NONE) {
                        sub_section = DFSection::IGNORED;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                }
            }

            continue;
        }

        // Look for close of section
        if(line.find('}') != std::string::npos) {
            switch(main_section) {
            case DFSection::VERTICES:
                if(vertices.size() != data_count) {
                    throw std::runtime_error("incorrect number of vertices");
                }
                break;
            case DFSection::EDGES:
                if(edges.size() != data_count) {
                    throw std::runtime_error("incorrect number of vertices");
                }
                break;
            case DFSection::FACES:
                if(faces.size() != data_count) {
                    throw std::runtime_error("incorrect number of faces");
                }
                break;
            case DFSection::ELEMENTS:
                if(elements.size() != data_count) {
                    throw std::runtime_error("incorrect number of elements");
                }
                break;
            default:
                break;
            }

            // Close section
            if(sub_section != DFSection::NONE) {
                sub_section = DFSection::NONE;
            } else if(main_section != DFSection::NONE) {
                main_section = DFSection::NONE;
            } else if(in_data_block) {
                in_data_block = false;
            } else {
                throw std::runtime_error("incorrect nesting of blocks");
            }

            continue;
        }

        // Look for key data pairs
        if(line.find('=') != std::string::npos) {
            auto base_regex = std::regex("([a-zA-Z]+)\\s+=\\s+([\\S ]+)");
            std::smatch base_match;
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto key = base_match[1].str();
                auto value = allpix::trim(base_match[2].str());

                // Filter correct electric field type
                if(main_section == DFSection::INFO) {
                    if(key == "dimension" && (std::stoul(value) != 3 && std::stoul(value) != 2)) {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "dimension" && (std::stoul(value) == 3 || std::stoul(value) == 2)) {
                        dimension = std::stoul(value);
                    }
                }
            }
            continue;
        }

        // Handle data
        std::stringstream sstr(line);
        switch(main_section) {
        case DFSection::HEADER:
            if(line != "DF-ISE text") {
                throw std::runtime_error("incorrect format, file does not have 'DF-ISE text' header");
            }
        case DFSection::INFO:
            break;
        case DFSection::VERTICES: {
            // Read vertex points
            if(dimension == 3) {
                point.x = -1.0;
                point.y = -1.0;
                point.z = -1.0;
                while(sstr >> point.x >> point.y >> point.z) {
                    vertices.push_back(point);
                    tree->Fill();
                }
            }
            if(dimension == 2) {
                point.x = -1.0;
                point.y = -1.0;
                point.z = -1.0;
                while(sstr >> point.y >> point.z) {
                    vertices.push_back(point);
                    tree->Fill();
                }
            }
        } break;
        case DFSection::EDGES: {
            // Read edges
            std::pair<long unsigned int, long unsigned int> edge;
            while(sstr >> edge.first >> edge.second) {
                if(edge.first >= vertices.size() || edge.second >= vertices.size()) {
                    throw std::runtime_error("vertex index is higher than number of vertices");
                }
                edges.push_back(edge);
            }
        } break;
        case DFSection::FACES: {
            // Get vertex indices for every face
            size_t n;
            sstr >> n;
            std::vector<long unsigned int> face;
            for(size_t i = 0; i < n; ++i) {
                long edge_idx;
                sstr >> edge_idx;

                bool swap = false;
                if(edge_idx < 0) {
                    edge_idx = -edge_idx - 1;
                    swap = true;
                }

                if(edge_idx >= static_cast<long>(edges.size())) {
                    throw std::runtime_error("edge index is higher than number of edges");
                }

                auto edge = edges[static_cast<size_t>(edge_idx)];

                if(swap) {
                    std::swap(edge.first, edge.second);
                }
                if(!face.empty() && face.back() == edge.second) {
                    std::swap(edge.first, edge.second);
                }

                face.push_back(edge.first);
                face.push_back(edge.second);
            }

            // Check first
            if(face.front() != face.back()) {
                std::swap(face.front(), face.back());
            }

            // Remove duplicates
            auto iter = std::unique(face.begin(), face.end());
            face.erase(iter, face.end());
            face.pop_back();

            faces.push_back(face);
        } break;
        case DFSection::ELEMENTS: {
            int k;
            sstr >> k;
            std::vector<long unsigned int> element;

            size_t size = 0;
            switch(k) {
            case 0: /* vertex */
                size = 1;
                break;
            case 1: /* Segment */
                size = 2;
                break;
            case 2: /* triangle */
                size = 3;
                break;
            case 3: /* rectangle */
                size = 4;
                break;
            case 5: /* tetrahedron */
                size = 4;
                break;
            case 6: /* pyramid */
                size = 5;
                break;
            case 7: /* prism */
                size = 5;
                break;
            case 8: /* brick */
                size = 6;
                break;
            default:
                throw std::runtime_error("element type " + std::to_string(k) + " is not supported");
            }

            for(size_t i = 0; i < size; ++i) {
                long element_idx;
                sstr >> element_idx;

                bool reverse = false;
                if(element_idx < 0) {
                    reverse = true;
                    element_idx = -element_idx - 1;
                }

                if(size == 3 || size == 2) {
                    if(element_idx >= static_cast<long>(edges.size())) {
                        throw std::runtime_error("edge index is higher than number of faces");
                    }
                    auto edge = edges[static_cast<size_t>(element_idx)];
                    if(reverse) {
                        std::swap(edge.first, edge.second);
                    }
                    element.push_back(edge.first);
                    element.push_back(edge.second);
                }
                if(size == 4) {
                    if(element_idx >= static_cast<long>(faces.size())) {
                        throw std::runtime_error("face index is higher than number of faces");
                    }
                    auto face = faces[static_cast<size_t>(element_idx)];
                    if(reverse) {
                        std::reverse(face.begin() + 1, face.end());
                    }
                    element.insert(element.end(), face.begin(), face.end());
                }
            }

            elements.push_back(element);
            break;
        }
        case DFSection::REGION: {
            if(sub_section != DFSection::ELEMENTS) {
                continue;
            }
            long unsigned int elem_idx;
            while(sstr >> elem_idx) {
                if(elem_idx >= elements.size()) {
                    throw std::runtime_error("element index is higher than number of elements");
                }

                regions_vertices[region].insert(
                    regions_vertices[region].end(), elements[elem_idx].begin(), elements[elem_idx].end());
            }

        } break;
        default:
            break;
        }
    }
    LOG_PROGRESS(INFO, "gridlines") << "Parsing grid file: done.";

    std::map<std::string, std::vector<Point>> ret_map;
    for(auto& name_region_vertices : regions_vertices) {
        auto region_vertices = name_region_vertices.second;

        std::sort(region_vertices.begin(), region_vertices.end());
        auto iter = std::unique(region_vertices.begin(), region_vertices.end());
        region_vertices.erase(iter, region_vertices.end());

        std::vector<Point> ret_vector;
        ret_vector.reserve(region_vertices.size());
        for(auto& vertex_idx : region_vertices) {
            ret_vector.push_back(vertices[vertex_idx]);
        }

        ret_map[name_region_vertices.first] = ret_vector;
    }

    if(mesh_tree) {
        std::string root_file_name = file_name + "_MESH_POINTS_TTREE.root";
        auto root_file = new TFile(root_file_name.c_str(), "RECREATE");
        tree->Write();
        root_file->Close();
    } else {
        tree->Delete();
    }

    return ret_map;
}

std::map<std::string, std::map<std::string, std::vector<Point>>>
mesh_converter::read_electric_field(const std::string& file_name) {
    std::ifstream file(file_name);
    if(!file) {
        throw std::runtime_error("file cannot be accessed");
    }

    // Get the total number of lines to parse and reset file to the start:
    auto num_lines = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
    file.clear();
    file.seekg(0, std::ios::beg);
    LOG(DEBUG) << "Field data file contains " << num_lines << " lines to parse";

    DFSection main_section = DFSection::HEADER;
    DFSection sub_section = DFSection::NONE;

    // std::map<std::string, std::vector<Point>> region_electric_field_map;
    std::map<std::string, std::map<std::string, std::vector<Point>>> region_electric_field_map;
    std::vector<double> region_electric_field_num;

    std::string region;
    std::string observable;
    long unsigned int dimension = 1;
    long unsigned int data_count = 0;
    bool in_data_block = false;
    long long num_lines_parsed = 0;
    while(!file.eof()) {
        std::string line;
        std::getline(file, line);
        line = allpix::trim(line);

        // Log the parsing progress:
        if(num_lines_parsed % 1000 == 0) {
            LOG_PROGRESS(INFO, "fieldlines") << "Parsing field data file: " << (100 * num_lines_parsed / num_lines) << "%";
        }
        num_lines_parsed++;

        if(line.empty()) {
            continue;
        }

        // Check if line with begin of section
        if(line.find('{') != std::string::npos) {
            // Search for new simple headers
            auto base_regex = std::regex("([a-zA-Z]+) \\{");
            std::smatch base_match;
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto header_string = base_match[1].str();
                LOG(TRACE) << "Opening section " << header_string;

                if(header_string == "Info") {
                    main_section = DFSection::INFO;
                } else if(header_string == "Data") {
                    in_data_block = true;
                } else {
                    if(main_section != DFSection::NONE) {
                        sub_section = DFSection::IGNORED;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                }
            }

            // Search for headers with data
            base_regex = std::regex("([a-zA-Z]+) \\((\\S+)\\) \\{");
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto header_string = base_match[1].str();
                auto header_data = base_match[2].str();

                if(header_string == "Dataset") {
                    std::string data_type = header_data.substr(1, header_data.size() - 2);
                    LOG(DEBUG) << "Opening dataset of type " << data_type;

                    if(data_type == "ElectricField") {
                        main_section = DFSection::ELECTRIC_FIELD;
                    } else if(data_type == "ElectrostaticPotential") {
                        main_section = DFSection::ELECTROSTATIC_POTENTIAL;
                    } else if(data_type == "DopingConcentration") {
                        main_section = DFSection::DOPING_CONCENTRATION;
                    } else if(data_type == "DonorConcentration") {
                        main_section = DFSection::DONOR_CONCENTRATION;
                    } else if(data_type == "AcceptorConcentration") {
                        main_section = DFSection::ACCEPTOR_CONCENTRATION;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                } else if(header_string == "Values") {
                    LOG(DEBUG) << "Opening value section with " << header_data << " entries";
                    sub_section = DFSection::VALUES;
                    data_count = std::stoul(header_data);
                } else {
                    if(main_section != DFSection::NONE) {
                        sub_section = DFSection::IGNORED;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                }
            }

            continue;
        }

        // Look for key data pairs
        if(line.find('=') != std::string::npos) {
            auto base_regex = std::regex("([a-zA-Z]+)\\s+=\\s+([\\S ]+)");
            std::smatch base_match;
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto key = base_match[1].str();
                auto value = allpix::trim(base_match[2].str());

                // Regular expression for matching validity region:
                base_regex = std::regex("\\[\\s+\"([-\\w\\.]+)\"\\s+\\]");
                if(key == "validity") {
                    // Ignore any electric field valid for multiple regions
                    if(std::regex_match(value, base_match, base_regex) && base_match.ready()) {
                        region = base_match[1].str();
                    } else {
                        LOG(INFO) << "Could not determine validity region for string \"" << value << "\", ignoring.";
                        main_section = DFSection::IGNORED;
                    }
                }

                // Only use vertex locations:
                if(key == "location" && value != "vertex") {
                    main_section = DFSection::IGNORED;
                }

                // Filter correct electric field type
                if(main_section == DFSection::ELECTRIC_FIELD) {
                    observable = "ElectricField";
                    if(key == "type" && value != "vector") {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "dimension" && (std::stoul(value) == 3 || std::stoul(value) == 2)) {
                        dimension = std::stoul(value);
                    }
                    if(key == "dimension" && (std::stoul(value) != 3 && std::stoul(value) != 2)) {
                        main_section = DFSection::IGNORED;
                    }
                }

                // Filter correct electric field type
                if(main_section == DFSection::ELECTROSTATIC_POTENTIAL) {
                    observable = "ElectrostaticPotential";
                    if(key == "type" && value != "scalar") {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "dimension" && std::stoul(value) == 1) {
                        dimension = std::stoul(value);
                    }
                    if(key == "dimension" && std::stoul(value) != 1) {
                        main_section = DFSection::IGNORED;
                    }
                }
                // Filter correct electric field type
                if(main_section == DFSection::DOPING_CONCENTRATION) {
                    observable = "DopingConcentration";
                    if(key == "type" && value != "scalar") {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "dimension" && std::stoul(value) == 1) {
                        dimension = std::stoul(value);
                    }
                    if(key == "dimension" && std::stoul(value) != 1) {
                        main_section = DFSection::IGNORED;
                    }
                }

                // Filter correct electric field type
                if(main_section == DFSection::DONOR_CONCENTRATION) {
                    observable = "DonorConcentration";
                    if(key == "type" && value != "scalar") {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "dimension" && std::stoul(value) == 1) {
                        dimension = std::stoul(value);
                    }
                    if(key == "dimension" && std::stoul(value) != 1) {
                        main_section = DFSection::IGNORED;
                    }
                }

                // Filter correct electric field type
                if(main_section == DFSection::ACCEPTOR_CONCENTRATION) {
                    observable = "AcceptorConcentration";
                    if(key == "type" && value != "scalar") {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "dimension" && std::stoul(value) == 1) {
                        dimension = std::stoul(value);
                    }
                    if(key == "dimension" && std::stoul(value) != 1) {
                        main_section = DFSection::IGNORED;
                    }
                }
            }
            continue;
        }

        // Look for close of section
        if(line.find('}') != std::string::npos) {

            if(main_section == DFSection::ELECTROSTATIC_POTENTIAL && sub_section == DFSection::VALUES) {
                if(data_count != region_electric_field_num.size()) {
                    throw std::runtime_error("incorrect number of electrostatic potential points");
                }

                for(size_t i = 0; i < region_electric_field_num.size(); i += 1) {
                    auto x = region_electric_field_num[i];
                    region_electric_field_map[region][observable].emplace_back(x, 0, 0);
                }

                region_electric_field_num.clear();
            }

            if(main_section == DFSection::ELECTRIC_FIELD && sub_section == DFSection::VALUES) {
                if(data_count != region_electric_field_num.size()) {
                    throw std::runtime_error("incorrect number of electric field points");
                }

                if(dimension == 3) {
                    for(size_t i = 0; i < region_electric_field_num.size(); i += 3) {
                        auto x = region_electric_field_num[i];
                        auto y = region_electric_field_num[i + 1];
                        auto z = region_electric_field_num[i + 2];
                        region_electric_field_map[region][observable].emplace_back(x, y, z);
                    }
                }

                if(dimension == 2) {
                    for(size_t i = 0; i < region_electric_field_num.size(); i += 2) {
                        auto x = region_electric_field_num[i];
                        auto y = region_electric_field_num[i + 1];
                        region_electric_field_map[region][observable].emplace_back(0, x, y);
                    }
                }

                region_electric_field_num.clear();
            }

            if(main_section == DFSection::DOPING_CONCENTRATION && sub_section == DFSection::VALUES) {
                if(data_count != region_electric_field_num.size()) {
                    throw std::runtime_error("incorrect number of points");
                }

                for(size_t i = 0; i < region_electric_field_num.size(); i += 1) {
                    auto x = region_electric_field_num[i];
                    region_electric_field_map[region][observable].emplace_back(x, 0, 0);
                }

                region_electric_field_num.clear();
            }

            if(main_section == DFSection::DONOR_CONCENTRATION && sub_section == DFSection::VALUES) {
                if(data_count != region_electric_field_num.size()) {
                    throw std::runtime_error("incorrect number of points");
                }

                for(size_t i = 0; i < region_electric_field_num.size(); i += 1) {
                    auto x = region_electric_field_num[i];
                    region_electric_field_map[region][observable].emplace_back(x, 0, 0);
                }

                region_electric_field_num.clear();
            }

            if(main_section == DFSection::ACCEPTOR_CONCENTRATION && sub_section == DFSection::VALUES) {
                if(data_count != region_electric_field_num.size()) {
                    throw std::runtime_error("incorrect number of points");
                }

                for(size_t i = 0; i < region_electric_field_num.size(); i += 1) {
                    auto x = region_electric_field_num[i];
                    region_electric_field_map[region][observable].emplace_back(x, 0, 0);
                }

                region_electric_field_num.clear();
            }

            // Close section
            if(sub_section != DFSection::NONE) {
                sub_section = DFSection::NONE;
            } else if(main_section != DFSection::NONE) {
                main_section = DFSection::NONE;
            } else if(in_data_block) {
                in_data_block = false;
            } else {
                throw std::runtime_error("incorrect nesting of blocks");
            }

            continue;
        }

        // Handle data
        if((main_section == DFSection::ELECTRIC_FIELD || main_section == DFSection::ELECTROSTATIC_POTENTIAL ||
            main_section == DFSection::DOPING_CONCENTRATION || main_section == DFSection::DONOR_CONCENTRATION ||
            main_section == DFSection::ACCEPTOR_CONCENTRATION) &&
           sub_section == DFSection::VALUES) {
            std::stringstream sstr(line);
            double num;
            while(sstr >> num) {
                region_electric_field_num.push_back(num);
            }
        }
    }
    LOG_PROGRESS(INFO, "fieldlines") << "Parsing field data file: done.";

    return region_electric_field_map;
}
