/**
 * @file
 * @brief Implementation of [DepositionPrimaries] module
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DepositionGeneratorModule.hpp"
#include "ActionInitializationPrimaries.hpp"
#include "PrimariesGeneratorAction.hpp"

#include "tools/geant4/MTRunManager.hpp"
#include "tools/geant4/RunManager.hpp"
#include "tools/geant4/geant4.h"

#include "core/utils/log.h"

using namespace allpix;

DepositionGeneratorModule::DepositionGeneratorModule(Configuration& config,
                                                     Messenger* messenger,
                                                     GeometryManager* geo_manager)
    : DepositionGeant4Module(config, messenger, geo_manager) {

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Do *not* waive sequence requirement - we're reading from file and this should happen sequentially
    waive_sequence_requirement(false);

    file_model_ = config_.get<FileModel>("model");

    // Force source type and position:
    config_.set("source_type", "cosmics"); // FIXME
    config_.set("source_position", ROOT::Math::XYZPoint());

    // Add the particle source position to the geometry
    geo_manager_->addPoint(config_.get<ROOT::Math::XYZPoint>("source_position", ROOT::Math::XYZPoint()));
}

void DepositionGeneratorModule::initialize() {
    reader_ = std::make_shared<PrimariesReader>(config_);

    DepositionGeant4Module::initialize();
}

void DepositionGeneratorModule::initialize_g4_action() {

    auto* action_initialization = new ActionInitializationPrimaries<PrimariesGeneratorAction>(config_, reader_);
    run_manager_g4_->SetUserInitialization(action_initialization);
}
