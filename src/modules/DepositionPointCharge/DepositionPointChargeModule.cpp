/**
 * @file
 * @brief Implementation of a module to deposit charges at a specific point
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DepositionPointChargeModule.hpp"

#include <cmath>
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
    config_.setDefault("model", "point");

    // Read model
    auto model = config_.get<std::string>("model");
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);
    if(model == "point") {
        model_ = DepositionModel::POINT;
    } else if(model == "scan") {
        model_ = DepositionModel::SCAN;
    } else {
        throw InvalidValueError(config_, "model", "Invalid deposition model, only 'point' and 'scan' are supported.");
    }
}

void DepositionPointChargeModule::init() {

    auto model = detector_->getModel();

    if(model_ == DepositionModel::SCAN) {
        // Get the config manager and retrieve total number of events:
        ConfigManager* conf_manager = getConfigManager();
        unsigned int events = conf_manager->getGlobalConfiguration().get<unsigned int>("number_of_events");
        root_ = static_cast<unsigned int>(std::round(std::cbrt(events)));
        if(events != root_ * root_ * root_) {
            LOG(WARNING) << "Number of events is no perfect cube, pixel cell volume cannot fully be covered in scan. "
                         << "Closest cube is " << root_ * root_ * root_;
        }

        // Calculate voxel size:
        voxel_ = ROOT::Math::XYZVector(
            model->getPixelSize().x() / root_, model->getPixelSize().y() / root_, model->getSensorSize().z() / root_);
        LOG(INFO) << "Voxel size for scan of pixel volume: " << Units::display(voxel_, {"um", "mm"});
    }
}

void DepositionPointChargeModule::run(unsigned int event) {

    // Vector of deposited charges and their "MCParticle"
    std::vector<DepositedCharge> charges;
    std::vector<MCParticle> mcparticles;

    // Local and global position of the MCParticle
    ROOT::Math::XYZPoint position_local;
    if(model_ == DepositionModel::SCAN) {
        auto model = detector_->getModel();
        // Center the volume to be scanned in the center of the sensor,
        // reference point is lower left corner of one pixel volume
        auto ref =
            model->getGridSize() / 2.0 - ROOT::Math::XYZVector(model->getPixelSize().x(), model->getPixelSize().y(), 0);
        position_local = ROOT::Math::XYZPoint(voxel_.x() * ((event - 1) % root_),
                                              voxel_.y() * (((event - 1) / root_) % root_),
                                              voxel_.z() * (((event - 1) / root_ / root_) % root_)) +
                         ref;
    } else {
        position_local = config_.get<ROOT::Math::XYZPoint>("position");
    }

    LOG(DEBUG) << "Position (local coordinates): " << Units::display(position_local, {"um"});
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
