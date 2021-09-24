/**
 * @file
 * @brief Implementation of [DepositionCosmics] module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DepositionCosmicsModule.hpp"
#include "CosmicsGeneratorActionG4.hpp"

#include "tools/geant4/MTRunManager.hpp"
#include "tools/geant4/RunManager.hpp"
#include "tools/geant4/geant4.h"

#include "../DepositionGeant4/ActionInitializationG4.hpp"

#include <filesystem>
#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

DepositionCosmicsModule::DepositionCosmicsModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : DepositionGeant4Module(config, messenger, geo_manager) {

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Register lookup path for CRY data files:
    if(config_.has("data_path")) {
        auto path = config_.getPath("data_path", true);
        if(!std::filesystem::is_directory(path)) {
            throw InvalidValueError(config_, "data_path", "path does not point to a directory");
        }
        LOG(TRACE) << "Registered CRY data path from configuration: " << path;
    } else {
        if(std::filesystem::is_directory(ALLPIX_CRY_DATA_DIRECTORY)) {
            config_.set<std::string>("data_path", ALLPIX_CRY_DATA_DIRECTORY);
            LOG(TRACE) << "Registered CRY data path from system: " << ALLPIX_CRY_DATA_DIRECTORY;
        } else {
            const char* data_dirs_env = std::getenv("XDG_DATA_DIRS");
            if(data_dirs_env == nullptr || strlen(data_dirs_env) == 0) {
                data_dirs_env = "/usr/local/share/:/usr/share/:";
            }

            std::vector<std::string> data_dirs = split<std::string>(data_dirs_env, ":");
            for(auto data_dir : data_dirs) {
                if(data_dir.back() != '/') {
                    data_dir += "/";
                }
                data_dir += std::string(ALLPIX_PROJECT_NAME) + std::string("/data");

                if(std::filesystem::is_directory(data_dir)) {
                    config_.set<std::string>("data_path", data_dir);
                    LOG(TRACE) << "Registered CRY data path from XDG_DATA_DIRS: " << data_dir;
                } else {
                    throw ModuleError("Cannot find CRY data files, provide them in the configuration, via XDG_DATA_DIRS or "
                                      "in system direcory " +
                                      std::string(ALLPIX_CRY_DATA_DIRECTORY));
                }
            }
        }
    }

    // Build configuration for CRY - it expects a single string with all tokens separated by whitespace:
    std::string cry_config;

    config_.set<std::string>("_cry_config", cry_config);
}

void DepositionCosmicsModule::initialize_g4_action() {
    auto* action_initialization =
        new ActionInitializationG4<CosmicsGeneratorActionG4, GeneratorActionInitializationMaster>(config_, this);
    run_manager_g4_->SetUserInitialization(action_initialization);
}
