#include <iomanip>

#include "MeshParser.hpp"

using namespace mesh_converter;

std::shared_ptr<MeshParser> MeshParser::factory(const allpix::Configuration& config) {
    auto parser = config.get<std::string>("parser", "df-ise");
    std::transform(parser.begin(), parser.end(), parser.begin(), ::tolower);

    if(parser == "df-ise" || parser == "dfise") {
        return std::make_shared<DFISEParser>();
    } else {
        throw allpix::InvalidValueError(config, "parser", "Unknown parser type");
    }
}

std::vector<Point> MeshParser::getMesh(const std::string& file, const std::vector<std::string>& regions) {
    std::vector<Point> points;

    auto region_grid = read_meshes(file);
    LOG(INFO) << "Grid sizes for all regions:";
    for(auto& reg : region_grid) {
        LOG(INFO) << "\t" << std::left << std::setw(25) << reg.first << " " << reg.second.size();
    }

    // Append all grid regions to the mesh:
    for(const auto& region : regions) {
        if(region_grid.find(region) != region_grid.end()) {
            points.insert(points.end(), region_grid[region].begin(), region_grid[region].end());
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

    std::vector<Point> field;
    auto region_fields = read_fields(file);
    LOG(INFO) << "Field sizes for all regions and observables:";
    for(auto& reg : region_fields) {
        LOG(INFO) << " " << reg.first << ":";
        for(auto& fld : reg.second) {
            LOG(INFO) << "\t" << std::left << std::setw(25) << fld.first << " " << fld.second.size();
        }
    }

    // Append all field regions to the field vector:
    for(const auto& region : regions) {
        if(region_fields.find(region) != region_fields.end() &&
           region_fields[region].find(observable) != region_fields[region].end()) {
            field.insert(field.end(), region_fields[region][observable].begin(), region_fields[region][observable].end());
        } else {
            throw std::runtime_error("Region \"" + region + "\" with observable \"" + observable +
                                     "\" not found in field file");
        }
    }

    if(field.empty()) {
        throw std::runtime_error("Empty observable data");
    }
    LOG(DEBUG) << "Field with " << field.size() << " points";

    return field;
}
