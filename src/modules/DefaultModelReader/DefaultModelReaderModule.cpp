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
    if(config.has("chip_size")) {
        model->setChipSize(config.get<XYZVector>("chip_size"));
    }
    if(config.has("chip_offset")) {
        model->setChipOffset(config.get<XYZVector>("chip_offset"));
    }
    if(config.has("sensor_size")) {
        model->setSensorSize(config.get<XYZVector>("sensor_size"));
    }
    if(config.has("sensor_offset")) {
        model->setSensorOffset(config.get<XYVector>("sensor_offset"));
    }
    if(config.has("pcb_size")) {
        model->setPCBSize(config.get<XYZVector>("pcb_size"));
    }

    // Excess around the pixel grid
    if(config.has("sensor_excess_top")) {
        model->setGuardRingExcessTop(config.get<double>("sensor_excess_top"));
    }
    if(config.has("sensor_excess_bottom")) {
        model->setGuardRingExcessBottom(config.get<double>("sensor_excess_bottom"));
    }
    if(config.has("sensor_excess_left")) {
        model->getGuardRingExcessLeft(config.get<double>("sensor_excess_left"));
    }
    if(config.has("sensor_excess_right")) {
        model->setGuardRingExcessRight(config.get<double>("sensor_excess_right"));
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
