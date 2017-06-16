#include "DefaultModelReaderModule.hpp"

#include <fstream>
#include <string>
#include <utility>

#include <Math/Vector3D.h>

#include "core/config/ConfigReader.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"

#include "tools/ROOT.h"

using namespace allpix;

// Constructors
DefaultModelReaderModule::DefaultModelReaderModule(Configuration config, Messenger*, GeometryManager* geo_mgr)
    : Module(config), config_(std::move(config)), geo_mgr_(geo_mgr) {
    // construct reader
    ConfigReader reader;

    // get standard paths from geometry reader
    std::vector<std::string> paths;
    if(config_.has("model_paths")) {
        paths = config_.getPathArray("model_paths", true);
    }
    auto global_paths = geo_mgr_->getModelsPath();
    paths.insert(paths.end(), global_paths.begin(), global_paths.end());

    LOG(TRACE) << "Reading model files";
    // add all the paths to the reader
    for(auto& path : paths) {
        // Check if file or directory
        if(allpix::path_is_directory(path)) {
            std::vector<std::string> sub_paths = allpix::get_files_in_directory(path);
            for(auto& sub_path : sub_paths) {
                // accept only with correct model suffix
                // FIXME .ini is not a good suffix for default models
                std::string suffix(ALLPIX_MODEL_SUFFIX);
                if(sub_path.size() < suffix.size() || sub_path.substr(sub_path.size() - suffix.size()) != suffix) {
                    continue;
                }

                // add the sub directory path to the reader
                LOG(TRACE) << "Reading model " << sub_path;
                std::fstream file(sub_path);
                reader.add(file, sub_path);
            }
        } else {
            // Always a file because paths are already checked
            LOG(TRACE) << "Reading model " << path;
            std::fstream file(path);
            reader.add(file, path);
        }
    }

    // loop through all configurations and parse them
    LOG(TRACE) << "Parsing models";
    for(auto& model_config : reader.getConfigurations()) {
        if(geo_mgr_->hasModel(model_config.getName())) {
            // Skip models that we already loaded earlier higher in the chain
            LOG(DEBUG) << "Skipping overwritten model " + model_config.getName() << " in path "
                       << model_config.getFilePath();
            continue;
        }
        if(!geo_mgr_->needsModel(model_config.getName())) {
            // Also skip models that are not needed
            LOG(TRACE) << "Skipping not required model " + model_config.getName() << " in path "
                       << model_config.getFilePath();
            continue;
        }

        // Parse configuration
        geo_mgr_->addModel(parse_config(model_config));
    }
}

std::shared_ptr<HybridPixelDetectorModel> DefaultModelReaderModule::parse_config(const Configuration& config) {
    std::string model_name = config.getName();
    std::shared_ptr<HybridPixelDetectorModel> model = std::make_shared<HybridPixelDetectorModel>(model_name);

    using namespace ROOT::Math;

    // Pixel amount
    if(config.has("pixel_amount")) {
        model->setNPixels(config.get<DisplacementVector2D<Cartesian2D<int>>>("pixel_amount"));
    }
    // Size, positions and offsets
    if(config.has("pixel_size")) {
        model->setPixelSize(config.get<XYVector>("pixel_size"));
    }
    // Sensor thickness
    if(config.has("sensor_thickness")) {
        model->setSensorThickness(config.get<double>("sensor_thickness"));
    }
    if(config.has("sensor_size")) {
        // DEPRECATED
        model->setSensorThickness(config.get<XYZVector>("sensor_size").z());
    }
    if(config.has("sensor_offset")) {
        // DEPRECATED
        // model->setSensorOffset(config.get<XYVector>("sensor_offset"));
    }
    // Excess around the sensor from the pixel grid
    if(config.has("sensor_excess_top")) {
        model->setSensorExcessTop(config.get<double>("sensor_excess_top"));
    }
    if(config.has("sensor_excess_bottom")) {
        model->setSensorExcessBottom(config.get<double>("sensor_excess_bottom"));
    }
    if(config.has("sensor_excess_left")) {
        model->setSensorExcessLeft(config.get<double>("sensor_excess_left"));
    }
    if(config.has("sensor_excess_right")) {
        model->setSensorExcessRight(config.get<double>("sensor_excess_right"));
    }

    // Chip thickness
    if(config.has("chip_thickness")) {
        model->setChipThickness(config.get<double>("chip_thickness"));
    }
    if(config.has("chip_size")) {
        // DEPRECATED
        model->setChipThickness(config.get<XYZVector>("chip_size").z());
    }
    if(config.has("chip_offset")) {
        // DEPRECATED
        // model->setChipOffset(config.get<XYZVector>("chip_offset"));
    }
    // Excess around the chip from the pixel grid
    if(config.has("chip_excess_top")) {
        model->setChipExcessTop(config.get<double>("chip_excess_top"));
    }
    if(config.has("chip_excess_bottom")) {
        model->setChipExcessBottom(config.get<double>("chip_excess_bottom"));
    }
    if(config.has("chip_excess_left")) {
        model->setChipExcessLeft(config.get<double>("chip_excess_left"));
    }
    if(config.has("chip_excess_right")) {
        model->setChipExcessRight(config.get<double>("chip_excess_right"));
    }

    if(config.has("pcb_size")) {
        // DEPRECATED
        model->setPCBThickness(config.get<XYZVector>("pcb_size").z());
    }
    if(config.has("pcb_thickness")) {
        // DEPRECATED
        model->setPCBThickness(config.get<double>("pcb_thickness"));
    }
    // Excess around the chip from the pixel grid
    if(config.has("pcb_excess_top")) {
        model->setPCBExcessTop(config.get<double>("pcb_excess_top"));
    }
    if(config.has("pcb_excess_bottom")) {
        model->setPCBExcessBottom(config.get<double>("pcb_excess_bottom"));
    }
    if(config.has("pcb_excess_left")) {
        model->setPCBExcessLeft(config.get<double>("pcb_excess_left"));
    }
    if(config.has("pcb_excess_right")) {
        model->setPCBExcessRight(config.get<double>("pcb_excess_right"));
    }

    // Bump parameters
    if(config.has("bump_sphere_radius")) {
        model->setBumpSphereRadius(config.get<double>("bump_sphere_radius"));
    }
    if(config.has("bump_height")) {
        model->setBumpHeight(config.get<double>("bump_height"));
    }
    if(config.has("bump_cylinder_radius")) {
        model->setBumpCylinderRadius(config.get<double>("bump_cylinder_radius"));
    }
    if(config.has("bump_offset")) {
        model->setBumpOffset(config.get<XYVector>("bump_offset"));
    }

    return model;
}
