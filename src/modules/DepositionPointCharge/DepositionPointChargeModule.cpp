/**
 * @file
 * @brief Implementation of a module to deposit charges at a specific point
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DepositionPointChargeModule.hpp"

#include <string>
#include <utility>

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "objects/MCParticle.hpp"

using namespace allpix;

DepositionPointChargeModule::DepositionPointChargeModule(Configuration& config,
                                                         Messenger* messenger,
                                                         std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), messenger_(messenger) {

    // Set default value for the number of charges deposited
    config_.setDefault("number_of_charges", 1);
    config_.setDefault("position", ROOT::Math::XYZPoint(0., 0., 0.));
}

void DepositionPointChargeModule::run(unsigned int) {

    // Vector of deposited charges and their "MCParticle"
    std::vector<DepositedCharge> charges;
    std::vector<MCParticle> mcparticles;

    // Local and global position of the MCParticle
    auto position_local = config_.get<ROOT::Math::XYZPoint>("position");
    auto position_global = detector_->getGlobalPosition(position_local);

    // Start and stop position is the same for the MCParticle
    mcparticles.emplace_back(position_local, position_global, position_local, position_global, -1, 0.);
    LOG(DEBUG) << "Generated MCParticle at global position " << position_global << " in detector " << detector_->getName();

    auto carriers = config_.get<unsigned int>("number_of_charges");
    charges.emplace_back(position_local, position_global, CarrierType::ELECTRON, carriers, 0., &(mcparticles.back()));
    charges.emplace_back(position_local, position_global, CarrierType::HOLE, carriers, 0., &(mcparticles.back()));
    LOG(DEBUG) << "Deposited " << carriers << " charge carriers of both types at global position " << position_global
               << " in detector " << detector_->getName();

    // Dispatch the messages to the framework
    auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(charges), detector_);
    auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(mcparticles), detector_);
    messenger_->dispatchMessage(this, deposit_message);
    messenger_->dispatchMessage(this, mcparticle_message);
}
