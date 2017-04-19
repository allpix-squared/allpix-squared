/**
 * @author Neal Gauvin <neal.gauvin@cern.ch>
 * @author John Idarraga <idarraga@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ReadGeoDescription.hpp"

#include <fstream>
#include <memory>
#include <string>

#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "core/config/ConfigReader.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/module/exceptions.h"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/string.h"

#include "tools/ROOT.h"

using namespace allpix;
using namespace ROOT::Math;

// Constructors
ReadGeoDescription::ReadGeoDescription() : ReadGeoDescription(std::vector<std::string>()) {}
ReadGeoDescription::ReadGeoDescription(std::vector<std::string> paths) : models_() {
    // construct reader
    ConfigReader reader;

    // add standard paths
    paths.emplace_back(ALLPIX_MODEL_DIRECTORY);
    const char* data_dirs_env = std::getenv("XDG_DATA_DIRS");
    if(data_dirs_env == nullptr || strlen(data_dirs_env) == 0) {
        data_dirs_env = "/usr/local/share/:/usr/share/:";
    }
    std::vector<std::string> data_dirs = split<std::string>(data_dirs_env, ":");
    for(auto data_dir : data_dirs) {
        if(data_dir.back() != '/') {
            data_dir += "/";
        }

        paths.emplace_back(data_dir + ALLPIX_PROJECT_NAME);
    }

    LOG(INFO) << "Reading model files";
    // add all the paths to the reader
    for(auto& path : paths) {
        // check if file or directory
        // NOTE: silently ignore all others
        if(path_is_directory(path)) {
            std::vector<std::string> sub_paths = get_files_in_directory(path);
            for(auto& sub_path : sub_paths) {
                // accept only with correct model suffix
                std::string suffix(ALLPIX_MODEL_SUFFIX);
                if(sub_path.size() < suffix.size() || sub_path.substr(sub_path.size() - suffix.size()) != suffix) {
                    continue;
                }

                // add the sub directory path to the reader
                LOG(DEBUG) << "Reading model " << sub_path;
                std::fstream file(sub_path);
                reader.add(file, sub_path);
            }
        } else if(path_is_file(path)) {
            // add the path to the reader
            LOG(DEBUG) << "Reading model " << path;
            std::fstream file(path);
            reader.add(file, path);
        }
    }

    // loop through all configurations and parse them
    LOG(INFO) << "Parsing models";
    for(auto& config : reader.getConfigurations()) {
        if(models_.find(config.getName()) != models_.end()) {
            // skip models that we already loaded earlier higher in the chain
            LOG(DEBUG) << "Skipping overwritten model " + config.getName() << " in path " << config.getFilePath();
            continue;
        }

        // FIXME: only parse configs that are actually used
        models_[config.getName()] = parse_config(config);
    }
}

std::shared_ptr<PixelDetectorModel> ReadGeoDescription::parse_config(const Configuration& config) {
    std::string model_name = config.getName();
    std::shared_ptr<PixelDetectorModel> model = std::make_shared<PixelDetectorModel>(model_name);

    // pixel amount
    if(config.has("pixel_amount")) {
        model->setNPixels(config.get<ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>>("pixel_amount"));
    }
    // size, positions and offsets
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

    // excess for the guard rings
    if(config.has("sensor_gr_excess_htop")) {
        model->setGuardRingExcessTop(config.get<double>("sensor_gr_excess_htop"));
    }
    if(config.has("sensor_gr_excess_hbottom")) {
        model->setGuardRingExcessBottom(config.get<double>("sensor_gr_excess_hbottom"));
    }
    if(config.has("sensor_gr_excess_hleft")) {
        model->getGuardRingExcessLeft(config.get<double>("sensor_gr_excess_hleft"));
    }
    if(config.has("sensor_gr_excess_hright")) {
        model->setGuardRingExcessHRight(config.get<double>("sensor_gr_excess_hright"));
    }

    // bump parameters
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

// Return detector model
// FIXME: should we throw an error if it does not exists
std::shared_ptr<PixelDetectorModel> ReadGeoDescription::getDetectorModel(const std::string& name) const {
    if(models_.find(name) == models_.end()) {
        return nullptr;
    }
    return models_.at(name);
}
