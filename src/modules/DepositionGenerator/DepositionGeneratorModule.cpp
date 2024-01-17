/**
 * @file
 * @brief Implementation of the DepositionGenerator module
 *
 * @copyright Copyright (c) 2022-2024 CERN and the Allpix Squared authors.
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

// Reader modules:
#include "PrimariesReaderGenie.hpp"

#if ALLPIX_GENERATOR_HEPMC
#include "PrimariesReaderHepMC.hpp"
#endif

using namespace allpix;

DepositionGeneratorModule::DepositionGeneratorModule(Configuration& config,
                                                     Messenger* messenger,
                                                     GeometryManager* geo_manager)
    : DepositionGeant4Module(config, messenger, geo_manager) {

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Do *not* waive sequence requirement - we're reading from file and this should happen sequentially
    waive_sequence_requirement(false);

    file_model_ = config_.get<PrimariesReader::FileModel>("model");

    // Force source type and position:
    config_.set("source_type", "generator");
    // FIXME allow to shift gun position by setting this parameter?
    config_.set("source_position", ROOT::Math::XYZPoint());
    // Force number of particles to one, we are always reading a single generator event per event
    config_.set("number_of_particles", 1);

    // Add the particle source position to the geometry
    geo_manager_->addPoint(config_.get<ROOT::Math::XYZPoint>("source_position", ROOT::Math::XYZPoint()));
}

void DepositionGeneratorModule::initialize() {

    // Generate file reader instance of appropriate type
    if(file_model_ == PrimariesReader::FileModel::GENIE) {
        reader_ = std::make_shared<PrimariesReaderGenie>(config_);
    } else if(file_model_ >= PrimariesReader::FileModel::HEPMC) {
#if ALLPIX_GENERATOR_HEPMC
        reader_ = std::make_shared<PrimariesReaderHepMC>(config_);
#else
        throw InvalidValueError(config_, "model", "Framework has been built without support for HepMC data file model");
#endif
    } else {
        throw InvalidValueError(config_, "model", "Unsupported data file model");
    }

    // Call upstream initialization method
    DepositionGeant4Module::initialize();
}

void DepositionGeneratorModule::run(Event* event) {
    // Pass current event number to the reader instance
    reader_->set_event_num(event->number);

    // Call upstream run method
    DepositionGeant4Module::run(event);
}

void DepositionGeneratorModule::initialize_g4_action() {
    auto* action_initialization = new ActionInitializationPrimaries<PrimariesGeneratorAction>(config_, reader_);
    run_manager_g4_->SetUserInitialization(action_initialization);
}
