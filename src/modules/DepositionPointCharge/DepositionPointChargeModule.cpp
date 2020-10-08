/**
 * @file
 * @brief Implementation of a module to deposit charges at a specific point
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
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
#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "objects/MCParticle.hpp"

using namespace allpix;

DepositionPointChargeModule::DepositionPointChargeModule(Configuration& config,
                                                         Messenger* messenger,
                                                         std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();

    // Allow to use similar syntax as in DepositionGeant4:
    config_.setAlias("position", "source_position");

    // Set default value for the number of charges deposited
    config_.setDefault("number_of_charges", 1);
    config_.setDefault("number_of_steps", 100);
    config_.setDefault("position", ROOT::Math::XYZPoint(0., 0., 0.));
    config_.setDefault("source_type", "point");

    // Read type:
    auto type = config_.get<std::string>("source_type");
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    if(type == "point") {
        type_ = SourceType::POINT;
    } else if(type == "mip") {
        type_ = SourceType::MIP;
    } else {
        throw InvalidValueError(config_, "source_type", "Invalid deposition type, only 'point' and 'mip' are supported.");
    }

    // Read model
    auto model = config_.get<std::string>("model");
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);
    if(model == "fixed") {
        model_ = DepositionModel::FIXED;
    } else if(model == "scan") {
        model_ = DepositionModel::SCAN;
    } else if(model == "spot") {
        model_ = DepositionModel::SPOT;
        spot_size_ = config.get<double>("spot_size");
    } else {
        throw InvalidValueError(
            config_, "model", "Invalid deposition model, only 'fixed', 'scan' and 'spot' are supported.");
    }
}

void DepositionPointChargeModule::initialize() {

    auto model = detector_->getModel();

    // Set up the different source types
    if(type_ == SourceType::MIP) {
        // Calculate voxel size and ensure granularity is not zero:
        auto granularity = std::max(config_.get<unsigned int>("number_of_steps"), 1u);
        step_size_z_ = model->getSensorSize().z() / granularity;

        // We should deposit the equivalent of about 80 e/h pairs per micro meter (80`000 per mm):
        carriers_ = static_cast<unsigned int>(80000 * step_size_z_);
        LOG(INFO) << "Step size for MIP energy deposition: " << Units::display(step_size_z_, {"um", "mm"}) << ", depositing "
                  << carriers_ << " e/h pairs per step";
    } else {
        carriers_ = config_.get<unsigned int>("number_of_charges");
    }

    // Set up the different scan methods
    if(model_ == DepositionModel::SCAN) {
        // Get the config manager and retrieve total number of events:
        ConfigManager* conf_manager = getConfigManager();
        auto events = conf_manager->getGlobalConfiguration().get<unsigned int>("number_of_events");

        // Scan with points required 3D scanning, scan with MIPs only 2D:
        if(type_ == SourceType::MIP) {
            root_ = static_cast<unsigned int>(std::round(std::sqrt(events)));
            if(events != root_ * root_) {
                LOG(WARNING) << "Number of events is not a square, pixel cell volume cannot fully be covered in scan. "
                             << "Closest square is " << root_ * root_;
            }
            // Calculate voxel size:
            voxel_ = ROOT::Math::XYZVector(
                model->getPixelSize().x() / root_, model->getPixelSize().y() / root_, model->getSensorSize().z());
        } else {
            root_ = static_cast<unsigned int>(std::round(std::cbrt(events)));
            if(events != root_ * root_ * root_) {
                LOG(WARNING) << "Number of events is not a cube, pixel cell volume cannot fully be covered in scan. "
                             << "Closest cube is " << root_ * root_ * root_;
            }
            // Calculate voxel size:
            voxel_ = ROOT::Math::XYZVector(
                model->getPixelSize().x() / root_, model->getPixelSize().y() / root_, model->getSensorSize().z() / root_);
        }
        LOG(INFO) << "Voxel size for scan of pixel volume: " << Units::display(voxel_, {"um", "mm"});
    }
}

void DepositionPointChargeModule::run(Event* event) {

    ROOT::Math::XYZPoint position;
    auto model = detector_->getModel();

    auto get_position = [&]() {
        if(config_.getArray<double>("position").size() == 2) {
            auto tmp_pos = config_.get<ROOT::Math::XYPoint>("position");
            return ROOT::Math::XYZPoint(tmp_pos.x(), tmp_pos.y(), 0);
        } else {
            return config_.get<ROOT::Math::XYZPoint>("position");
        }
    };

    if(model_ == DepositionModel::FIXED) {
        // Fixed position as read from the configuration:
        position = get_position();
    } else if(model_ == DepositionModel::SCAN) {
        // Center the volume to be scanned in the center of the sensor,
        // reference point is lower left corner of one pixel volume
        auto ref = config_.get<ROOT::Math::XYZVector>("position") + model->getGridSize() / 2.0 + voxel_ / 2.0 -
                   ROOT::Math::XYZVector(
                       model->getPixelSize().x() / 2.0, model->getPixelSize().y() / 2.0, model->getSensorSize().z() / 2.0);
        LOG(DEBUG) << "Reference: " << ref;
        position = ROOT::Math::XYZPoint(voxel_.x() * static_cast<double>((event->number - 1) % root_),
                                        voxel_.y() * static_cast<double>(((event->number - 1) / root_) % root_),
                                        voxel_.z() * static_cast<double>(((event->number - 1) / root_ / root_) % root_)) +
                   ref;
    } else {
        // Calculate random offset from configured position
        auto shift = [&](auto size) {
            double dx = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
            double dy = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
            double dz = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
            return ROOT::Math::XYZVector(dx, dy, dz);
        };

        // Spot around the configured position
        position = get_position() + shift(config_.get<double>("spot_size"));
    }

    // Create charge carriers at requested position
    if(type_ == SourceType::MIP) {
        DepositLine(event, position);
    } else {
        DepositPoint(event, position);
    }
}

void DepositionPointChargeModule::DepositPoint(Event* event, const ROOT::Math::XYZPoint& position) {
    // Vector of deposited charges and their "MCParticle"
    std::vector<DepositedCharge> charges;
    std::vector<MCParticle> mcparticles;

    LOG(DEBUG) << "Position (local coordinates): " << Units::display(position, {"um", "mm"});
    // Cross-check calculated position to be within sensor:
    if(!detector_->isWithinSensor(position)) {
        LOG(DEBUG) << "Requested position is outside active sensor volume.";
        return;
    }

    auto position_global = detector_->getGlobalPosition(position);

    // Start and stop position is the same for the MCParticle
    mcparticles.emplace_back(position, position_global, position, position_global, -1, 0., 0.);
    LOG(DEBUG) << "Generated MCParticle at global position " << Units::display(position_global, {"um", "mm"})
               << " in detector " << detector_->getName();

    charges.emplace_back(position, position_global, CarrierType::ELECTRON, carriers_, 0., 0., &(mcparticles.back()));
    charges.emplace_back(position, position_global, CarrierType::HOLE, carriers_, 0., 0., &(mcparticles.back()));
    LOG(DEBUG) << "Deposited " << carriers_ << " charge carriers of both types at global position "
               << Units::display(position_global, {"um", "mm"}) << " in detector " << detector_->getName();

    // Dispatch the messages to the framework
    auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(mcparticles), detector_);
    messenger_->dispatchMessage(this, mcparticle_message, event);

    auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(charges), detector_);
    messenger_->dispatchMessage(this, deposit_message, event);
}

void DepositionPointChargeModule::DepositLine(Event* event, const ROOT::Math::XYZPoint& position) {
    auto model = detector_->getModel();

    // Vector of deposited charges and their "MCParticle"
    std::vector<DepositedCharge> charges;
    std::vector<MCParticle> mcparticles;

    // Cross-check calculated position to be within sensor:
    if(!detector_->isWithinSensor(ROOT::Math::XYZPoint(position.x(), position.y(), 0))) {
        LOG(DEBUG) << "Requested position is outside active sensor volume.";
        return;
    }

    // Start and end position of MCParticle:
    auto start_local = ROOT::Math::XYZPoint(position.x(), position.y(), -model->getSensorSize().z() / 2.0);
    auto end_local = ROOT::Math::XYZPoint(position.x(), position.y(), model->getSensorSize().z() / 2.0);
    auto start_global = detector_->getGlobalPosition(start_local);
    auto end_global = detector_->getGlobalPosition(end_local);

    // Create MCParticle:
    mcparticles.emplace_back(start_local, start_global, end_local, end_global, -1, 0., 0.);
    LOG(DEBUG) << "Generated MCParticle with start " << Units::display(start_global, {"um", "mm"}) << " and end "
               << Units::display(end_global, {"um", "mm"}) << " in detector " << detector_->getName();

    // Deposit the charge carriers:
    auto position_local = start_local;
    while(position_local.z() < model->getSensorSize().z() / 2.0) {
        position_local += ROOT::Math::XYZVector(0, 0, step_size_z_);
        auto position_global = detector_->getGlobalPosition(position_local);

        charges.emplace_back(
            position_local, position_global, CarrierType::ELECTRON, carriers_, 0., 0., &(mcparticles.back()));
        charges.emplace_back(position_local, position_global, CarrierType::HOLE, carriers_, 0., 0., &(mcparticles.back()));
        LOG(TRACE) << "Deposited " << carriers_ << " charge carriers of both types at global position "
                   << Units::display(position_global, {"um", "mm"}) << " in detector " << detector_->getName();
    }

    // Dispatch the messages to the framework
    auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(mcparticles), detector_);
    messenger_->dispatchMessage(this, mcparticle_message, event);

    auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(charges), detector_);
    messenger_->dispatchMessage(this, deposit_message, event);
}
