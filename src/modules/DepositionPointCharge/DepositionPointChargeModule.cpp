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
#include "core/module/Event.hpp"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "objects/MCParticle.hpp"

using namespace allpix;

DepositionPointChargeModule::DepositionPointChargeModule(Configuration& config,
                                                         Messenger*,
                                                         std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {

    // Set default value for the number of charges deposited
    config_.setDefault("number_of_charges", 1);
    config_.setDefault("number_of_steps", 100);
    config_.setDefault("position", ROOT::Math::XYZPoint(0., 0., 0.));
    config_.setDefault("model", "point");

    // Read model
    auto model = config_.get<std::string>("model");
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);
    if(model == "point") {
        model_ = DepositionModel::POINT;
    } else if(model == "scan") {
        model_ = DepositionModel::SCAN;
    } else if(model == "mip") {
        model_ = DepositionModel::MIP;
    } else {
        throw InvalidValueError(config_, "model", "Invalid deposition model, only 'point', 'scan' and `mip` are supported.");
    }
}

void DepositionPointChargeModule::init(std::mt19937_64&) {

    auto model = detector_->getModel();

    carriers_ = config_.get<unsigned int>("number_of_charges");
    if(model_ == DepositionModel::SCAN) {
        // Get the config manager and retrieve total number of events:
        ConfigManager* conf_manager = getConfigManager();
        auto events = conf_manager->getGlobalConfiguration().get<unsigned int>("number_of_events");
        root_ = static_cast<unsigned int>(std::round(std::cbrt(events)));
        if(events != root_ * root_ * root_) {
            LOG(WARNING) << "Number of events is no perfect cube, pixel cell volume cannot fully be covered in scan. "
                         << "Closest cube is " << root_ * root_ * root_;
        }

        // Calculate voxel size:
        voxel_ = ROOT::Math::XYZVector(
            model->getPixelSize().x() / root_, model->getPixelSize().y() / root_, model->getSensorSize().z() / root_);
        LOG(INFO) << "Voxel size for scan of pixel volume: " << Units::display(voxel_, {"um", "mm"});
    } else if(model_ == DepositionModel::MIP) {
        // Calculate voxel size:
        auto granularity = config_.get<unsigned int>("number_of_steps");
        voxel_ = ROOT::Math::XYZVector(0, 0, model->getSensorSize().z() / granularity);

        // We should deposit the equivalent of about 80 e/h pairs per micro meter (80`000 per mm):
        carriers_ = static_cast<unsigned int>(80000 * voxel_.z());
        LOG(INFO) << "Step size for MIP energy deposition: " << Units::display(voxel_.z(), {"um", "mm"}) << ", depositing "
                  << carriers_ << " e/h pairs per step";
    }
}

void DepositionPointChargeModule::run(Event* event) {

    if(model_ == DepositionModel::MIP) {
        DepositLine(event);
    } else {
        DepositPoint(event);
    }
}

void DepositionPointChargeModule::DepositPoint(Event* event) const {
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
        position_local = ROOT::Math::XYZPoint(voxel_.x() * ((event->number - 1) % root_),
                                              voxel_.y() * (((event->number - 1) / root_) % root_),
                                              voxel_.z() * (((event->number - 1) / root_ / root_) % root_)) +
                         ref;
    } else {
        position_local = config_.get<ROOT::Math::XYZPoint>("position");
    }

    LOG(DEBUG) << "Position (local coordinates): " << Units::display(position_local, {"um", "mm"});
    auto position_global = detector_->getGlobalPosition(position_local);

    // Start and stop position is the same for the MCParticle
    mcparticles.emplace_back(position_local, position_global, position_local, position_global, -1, 0.);
    LOG(DEBUG) << "Generated MCParticle at global position " << Units::display(position_global, {"um", "mm"})
               << " in detector " << detector_->getName();

    charges.emplace_back(position_local, position_global, CarrierType::ELECTRON, carriers_, 0., &(mcparticles.back()));
    charges.emplace_back(position_local, position_global, CarrierType::HOLE, carriers_, 0., &(mcparticles.back()));
    LOG(DEBUG) << "Deposited " << carriers_ << " charge carriers of both types at global position "
               << Units::display(position_global, {"um", "mm"}) << " in detector " << detector_->getName();

    // Dispatch the messages to the framework
    auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(charges), detector_);
    auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(mcparticles), detector_);
    event->dispatchMessage(deposit_message);
    event->dispatchMessage(mcparticle_message);
}

void DepositionPointChargeModule::DepositLine(Event* event) const {
    auto model = detector_->getModel();

    // Vector of deposited charges and their "MCParticle"
    std::vector<DepositedCharge> charges;
    std::vector<MCParticle> mcparticles;

    ROOT::Math::XYPoint position;
    if(config_.getArray<double>("position").size() == 2) {
        position = config_.get<ROOT::Math::XYPoint>("position");
    } else {
        position = static_cast<ROOT::Math::XYPoint>(config_.get<ROOT::Math::XYZPoint>("position"));
    }

    // Start and end position of MCParticle:
    auto start_local = ROOT::Math::XYZPoint(position.x(), position.y(), -model->getSensorSize().z() / 2.0);
    auto end_local = ROOT::Math::XYZPoint(position.x(), position.y(), model->getSensorSize().z() / 2.0);
    auto start_global = detector_->getGlobalPosition(start_local);
    auto end_global = detector_->getGlobalPosition(end_local);

    // Create MCParticle:
    mcparticles.emplace_back(start_local, start_global, end_local, end_global, -1, 0.);
    LOG(DEBUG) << "Generated MCParticle with start " << Units::display(start_global, {"um", "mm"}) << " and end "
               << Units::display(end_global, {"um", "mm"}) << " in detector " << detector_->getName();

    // Deposit the charge carriers:
    auto position_local = start_local;
    while(position_local.z() < model->getSensorSize().z() / 2.0) {
        position_local += voxel_;
        auto position_global = detector_->getGlobalPosition(position_local);

        charges.emplace_back(position_local, position_global, CarrierType::ELECTRON, carriers_, 0., &(mcparticles.back()));
        charges.emplace_back(position_local, position_global, CarrierType::HOLE, carriers_, 0., &(mcparticles.back()));
        LOG(TRACE) << "Deposited " << carriers_ << " charge carriers of both types at global position "
                   << Units::display(position_global, {"um", "mm"}) << " in detector " << detector_->getName();
    }

    // Dispatch the messages to the framework
    auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(charges), detector_);
    auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(mcparticles), detector_);
    event->dispatchMessage(deposit_message);
    event->dispatchMessage(mcparticle_message);
}
