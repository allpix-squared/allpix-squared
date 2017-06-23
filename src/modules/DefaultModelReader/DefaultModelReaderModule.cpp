#include "DefaultModelReaderModule.hpp"

#include <fstream>
#include <string>
#include <utility>

#include <Math/Vector3D.h>

#include "core/config/ConfigReader.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "tools/ROOT.h"

#include "core/geometry/HybridPixelDetectorModel.hpp"
#include "core/geometry/MonolithicPixelDetectorModel.hpp"

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

std::shared_ptr<DetectorModel> DefaultModelReaderModule::parse_config(const Configuration& config) {
    if(!config.has("type")) {
        LOG(ERROR) << "Model file " << config.getFilePath() << " does not provide a type parameter";
    }
    auto type = config.get<std::string>("type");

    // Instantiate the correct detector model
    if(type == "hybrid") {
        return std::make_shared<HybridPixelDetectorModel>(config);
    }
    if(type == "monolithic") {
        return std::make_shared<MonolithicPixelDetectorModel>(config);
    }

    LOG(ERROR) << "Model file " << config.getFilePath() << " type parameter is not valid";
    // FIXME: The model can probably be silently ignored if we have more model readers later
    throw InvalidValueError(config, "type", "model type is not supported");
}
