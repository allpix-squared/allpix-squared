/**
 * @file
 * @brief Implementation of DepositionCosmics module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
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
#include <sstream>
#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

DepositionCosmicsModule::DepositionCosmicsModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : DepositionGeant4Module(config, messenger, geo_manager) {

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Set defaults for configuration parameters:
    config_.setDefault("return_neutrons", true);
    config_.setDefault("return_protons", true);
    config_.setDefault("return_gammas", true);
    config_.setDefault("return_electrons", true);
    config_.setDefault("return_muons", true);
    config_.setDefault("return_pions", true);
    config_.setDefault("return_kaons", true);
    config_.setDefault("altitude", Units::get(0, "m"));
    config_.setDefault("min_particles", 1);
    config_.setDefault("max_particles", 1000000);

    // Force source type and position:
    config_.set("source_type", "cosmics");
    config_.set("source_position", ROOT::Math::XYZPoint());

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
                                      "in system directory " +
                                      std::string(ALLPIX_CRY_DATA_DIRECTORY));
                }
            }
        }
    }

    // Build configuration for CRY - it expects a single string with all tokens separated by whitespace:
    std::stringstream cry_config;

    cry_config << " returnNeutrons " << static_cast<int>(config_.get<bool>("return_neutrons")) << " returnProtons "
               << static_cast<int>(config_.get<bool>("return_protons")) << " returnGammas "
               << static_cast<int>(config_.get<bool>("return_gammas")) << " returnElectrons "
               << static_cast<int>(config_.get<bool>("return_electrons")) << " returnMuons "
               << static_cast<int>(config_.get<bool>("return_muons")) << " returnPions "
               << static_cast<int>(config_.get<bool>("return_pions")) << " returnKaons "
               << static_cast<int>(config_.get<bool>("return_kaons"));

    // Select altitude:
    auto altitude = config_.get<int>("altitude");
    if(altitude != 0 && altitude != 2100000 && altitude != 11300000) {
        throw InvalidValueError(config_, "altitude", "only altitudes of 0m, 2100m and 11300m are supported");
    }
    cry_config << " altitude " << static_cast<int>(Units::convert(altitude, "m"));

    cry_config << " nParticlesMin " << config_.get<unsigned int>("min_particles") << " nParticlesMax "
               << config_.get<unsigned int>("max_particles");

    // Calculate subbox length required from the maximum coordinates of the setup:
    LOG(DEBUG) << "Calculating subbox length from setup size";
    auto min = geo_manager_->getMinimumCoordinate();
    auto max = geo_manager_->getMaximumCoordinate();
    auto size = std::max(max.x() - min.x(), max.y() - min.y());
    auto size_meters = static_cast<unsigned int>(std::ceil(Units::convert(size, "m")));
    if(size_meters > 300) {
        throw ModuleError("Size of the setup too large, tabulated data only available for areas up to 300m");
    }
    LOG(DEBUG) << "Maximum side length (in x,y): " << Units::display(size, {"mm", "cm", "m"})
               << ", selecting subbox of size " << size_meters << "m";
    cry_config << " subboxLength " << size_meters;

    // _parmNames[CRYSetup::latitude]="latitude";
    // _parmNames[CRYSetup::date]="date";
    // _parmNames[CRYSetup::xoffset]="xoffset";
    // _parmNames[CRYSetup::yoffset]="yoffset";
    // _parmNames[CRYSetup::zoffset]="zoffset";

    config_.set<std::string>("_cry_config", cry_config.str());
}

void DepositionCosmicsModule::initialize_g4_action() {
    auto* action_initialization =
        new ActionInitializationG4<CosmicsGeneratorActionG4, GeneratorActionInitializationMaster>(config_, this);
    run_manager_g4_->SetUserInitialization(action_initialization);
}
