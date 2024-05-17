/**
 * @file
 * @brief Implementation of DepositionCosmics module
 *
 * @copyright Copyright (c) 2021-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DepositionCosmicsModule.hpp"
#include "CosmicsGeneratorActionG4.hpp"

#include "tools/geant4/MTRunManager.hpp"
#include "tools/geant4/RunManager.hpp"
#include "tools/geant4/geant4.h"

#include <G4Box.hh>
#include <G4LogicalVolume.hh>

#include "../DepositionGeant4/ActionInitializationG4.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

thread_local double DepositionCosmicsModule::cry_instance_time_simulated_ = 0;

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
    config_.setDefault("latitude", 53.0);
    config_.setDefault("date", "12-31-2020");
    config_.setDefault("reset_particle_time", false);

    // Force source type and position:
    config_.set("source_type", "cosmics");
    config_.set("source_position", ROOT::Math::XYZPoint());

    // Add the particle source position to the geometry
    geo_manager_->addPoint(config_.get<ROOT::Math::XYZPoint>("source_position", ROOT::Math::XYZPoint()));

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

            auto data_dirs = split<std::filesystem::path>(data_dirs_env, ":");
            for(auto data_dir : data_dirs) {
                data_dir /= std::filesystem::path(ALLPIX_PROJECT_NAME) / "data";
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

    // Get G4 world size to check the subbox size
    double min_world_size_meters{0};
    auto world_log_volume = geo_manager_->getExternalObject<G4LogicalVolume>("", "world_log");
    if(world_log_volume != nullptr) {
        auto* world_box = static_cast<G4Box*>(world_log_volume->GetSolid());
        min_world_size_meters = 2e-3 * std::min(world_box->GetXHalfLength(), world_box->GetYHalfLength());
    }

    if(!config_.has("area")) {
        // Calculate subbox length required from the maximum coordinates of the setup:
        // Use maximum coordinate instead of setup size to make sure that off-center setups are fully covered
        LOG(DEBUG) << "Calculating subbox length from setup size";
        auto min = geo_manager_->getMinimumCoordinate();
        auto max = geo_manager_->getMaximumCoordinate();
        std::vector<double> max_candidates = {
            std::fabs(max.x()), std::fabs(max.y()), std::fabs(min.x()), std::fabs(min.y())};
        auto max_abs_coord = *std::max_element(begin(max_candidates), end(max_candidates));

        // Round margins up to 10 cm
        auto size_meters = static_cast<double>(std::ceil(Units::convert(2 * max_abs_coord, "m") * 10.0)) / 10.0;
        if(size_meters > 300) {
            throw ModuleError("Size of the setup too large, tabulated data only available for areas up to 300m");
        }

        if(min_world_size_meters < size_meters) {
            LOG(WARNING) << "Calculated subbox size does not fit in Geant4 world; undefined behaviour possible for "
                            "primaries generated outside the world box";
        }

        LOG(DEBUG) << "Maximum absolute coordinate (in x,y): " << Units::display(max_abs_coord, {"mm", "cm", "m"})
                   << ", selecting subbox of size " << size_meters << "m";
        cry_config << " subboxLength " << size_meters;
    } else {
        auto area = Units::convert(config_.get<double>("area"), "m");
        if(area > 300) {
            throw InvalidValueError(config_, "area", "only areas with side lengths of up to 300m are supported");
        }
        LOG(DEBUG) << "Configuring subbox of size " << area << "m from configuration parameter";
        if(min_world_size_meters < area) {
            LOG(WARNING) << "User-defined subbox size does not fit in Geant4 world; undefined behaviour possible for "
                            "primaries generated outside the world box";
        }
        cry_config << " subboxLength " << area;
    }

    auto latitude = config_.get<double>("latitude");
    if(latitude < -90 || latitude > 90) {
        throw InvalidValueError(config_, "latitude", "latitude has to be between 90.0 (north pole) and -90.0 (south pole)");
    }
    cry_config << " latitude " << latitude;
    cry_config << " date " << config_.get<std::string>("date");

    // Set CRY configuration string as internal key:
    config_.set<std::string>("_cry_config", cry_config.str());
}

void DepositionCosmicsModule::initialize_g4_action() {
    auto* action_initialization =
        new ActionInitializationG4<CosmicsGeneratorActionG4, GeneratorActionInitializationMaster>(config_);
    run_manager_g4_->SetUserInitialization(action_initialization);
}

void DepositionCosmicsModule::finalizeThread() {
    // Call base class thread finalization:
    DepositionGeant4Module::finalizeThread();

    LOG(DEBUG) << "CRY instance reports simulation time of "
               << Units::display(cry_instance_time_simulated_, {"us", "ms", "s"});
    std::lock_guard<std::mutex> lock{stats_mutex_};
    total_time_simulated_ += cry_instance_time_simulated_;
}

void DepositionCosmicsModule::finalize() {
    LOG(STATUS) << "Total simulated time in CRY: " << Units::display(total_time_simulated_, {"us", "ms", "s"});
    config_.set("total_time_simulated", total_time_simulated_);

    // Call base class finalization:
    DepositionGeant4Module::finalize();
}
