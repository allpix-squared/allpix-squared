/**
 * @file
 * @brief Implementation of a module to deposit charges at a specific point
 *
 * @copyright Copyright (c) 2018-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Allow to use similar syntax as in DepositionGeant4:
    config_.setAlias("position", "source_position");

    // Set default value for the number of charges deposited
    config_.setDefault("position", ROOT::Math::XYZPoint(0., 0., 0.));
    config_.setDefault("source_type", SourceType::POINT);

    // Plotting parameters
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<int>("output_plots_bins_per_um", 1);

    // Read type and model:
    type_ = config_.get<SourceType>("source_type");
    model_ = config_.get<DepositionModel>("model");

    // Read spot size
    if(model_ == DepositionModel::SPOT) {
        spot_size_ = config.get<double>("spot_size");
    }

    // Read position
    if(config_.getArray<double>("position").size() == 2) {
        auto tmp_pos = config_.get<ROOT::Math::XYPoint>("position");
        position_ = ROOT::Math::XYZVector(tmp_pos.x(), tmp_pos.y(), 0);
    } else {
        position_ = config_.get<ROOT::Math::XYZVector>("position");
    }
}

void DepositionPointChargeModule::initialize() {

    detector_model_ = detector_->getModel();
    output_plots_ = config_.get<bool>("output_plots");
    output_plots_bins_per_um_ = config_.get<int>("output_plots_bins_per_um");

    // Set up the different source types
    if(type_ == SourceType::MIP) {
        config_.setDefault("number_of_steps", 100);
        config_.setDefault("number_of_charges", 80000);

        // Calculate voxel size and ensure granularity is not zero:
        auto granularity = std::max(config_.get<unsigned int>("number_of_steps"), 1u);
        step_size_z_ = detector_model_->getSensorSize().z() / granularity;
        // NOTE: probably want to change this. To go along the axis we deposit along... But how?

        // We should deposit the equivalent of about 80 e/h pairs per micro meter (80`000 per mm):
        auto eh_per_um = config_.get<unsigned int>("number_of_charges");
        carriers_ = static_cast<unsigned int>(eh_per_um * step_size_z_);
        LOG(INFO) << "Step size for MIP energy deposition: " << Units::display(step_size_z_, {"um", "mm"}) << ", depositing "
                  << carriers_ << " e/h pairs per step (" << Units::display(eh_per_um, "/um") << ")";

        mip_direction_ = config_.get<ROOT::Math::XYZVector>("mip_direction", ROOT::Math::XYZVector(0.0, 0.0, 1.0)).Unit();

        // Check if the number of charge carriers is larger than zero
        if(carriers_ == 0) {
            throw InvalidValueError(config_,
                                    "number_of_steps",
                                    "Number of charge carriers deposited per step is zero due to a large step number or "
                                    "small number of e/h pairs per um");
        }

    } else {
        config_.setDefault("number_of_charges", 1);
        carriers_ = config_.get<unsigned int>("number_of_charges");
    }

    // Set up the different scan methods
    if(model_ == DepositionModel::SCAN) {
        // Get the config manager and retrieve total number of events:
        ConfigManager* conf_manager = getConfigManager();
        auto events = conf_manager->getGlobalConfiguration().get<unsigned int>("number_of_events");
        scan_coordinates_ = config_.getArray<std::string>("scan_coordinates", {"x", "y", "z"});

        scan_x_ = std::find(scan_coordinates_.begin(), scan_coordinates_.end(), "x") != scan_coordinates_.end();
        scan_y_ = std::find(scan_coordinates_.begin(), scan_coordinates_.end(), "y") != scan_coordinates_.end();
        scan_z_ = std::find(scan_coordinates_.begin(), scan_coordinates_.end(), "z") != scan_coordinates_.end();

        size_t no_of_coordinates = scan_coordinates_.size();

        // If MIP, and along one of the cardinal directions: don't do a scan in that direction, as the MIP goes along it
        // anyway
        if(scan_x_ && mip_direction_ == ROOT::Math::XYZVector(1.0, 0.0, 0.0)) {
            scan_x_ = false;
            no_of_coordinates--;
            LOG(WARNING) << "MIP shot in the x-direction; don't scan in x";
        }
        if(scan_y_ && mip_direction_ == ROOT::Math::XYZVector(0.0, 1.0, 0.0)) {
            scan_y_ = false;
            no_of_coordinates--;
            LOG(WARNING) << "MIP shot in the y-direction; don't scan in y";
        }
        if(scan_z_ && mip_direction_ == ROOT::Math::XYZVector(0.0, 0.0, 1.0)) {
            scan_z_ = false;
            no_of_coordinates--;
            LOG(WARNING) << "MIP shot in the z-direction; don't scan in z";
        }

        if(no_of_coordinates < 1) {
            LOG(WARNING) << "A scan will not be performed, but all MIPs will be along the same line.";
        }

        if(no_of_coordinates > 3 || !(scan_x_ || scan_y_ || scan_z_) ||
           (no_of_coordinates == 3 && !(scan_x_ && scan_y_ && scan_z_))) {
            throw InvalidValueError(
                config_,
                "scan_coordinates",
                "The coordinates must be a combination of x, y, and z, and the number of coordinates cannot exceed 3.");
        }

        // Scan with points required 3D scanning, scan with MIPs only 2D:
        root_ = events;
        if(no_of_coordinates == 2) {
            // Throw if we don't have a valid combination. Need 2 valid entries; x y, x z, or y z
            root_ = static_cast<unsigned int>(std::lround(std::sqrt(events)));
            if(events != root_ * root_) {
                LOG(WARNING) << "Number of events is not a square, pixel cell volume cannot fully be covered in scan. "
                             << "Closest square is " << root_ * root_;
            }
            if(!((scan_x_ && scan_y_) || (scan_x_ && scan_z_) || (scan_y_ && scan_z_))) {
                throw InvalidValueError(config_,
                                        "scan_coordinates",
                                        "The coordinates must be x, y, or z, and a coordinate must not be repeated");
            }
        } else if(no_of_coordinates == 3) {
            root_ = static_cast<unsigned int>(std::lround(std::cbrt(events)));
            if(events != root_ * root_ * root_) {
                LOG(WARNING) << "Number of events is not a cube, pixel cell volume cannot fully be covered in scan. "
                             << "Closest cube is " << root_ * root_ * root_;
            }
        }
        // Calculate voxel size:
        voxel_ = ROOT::Math::XYZVector(detector_model_->getPixelSize().x() / (scan_x_ ? root_ : 1.0),
                                       detector_model_->getPixelSize().y() / (scan_y_ ? root_ : 1.0),
                                       detector_model_->getSensorSize().z() / (scan_z_ ? root_ : 1.0));
        LOG(INFO) << "Voxel size for scan of pixel volume: " << Units::display(voxel_, {"um", "mm"});
    }

    if(output_plots_) {
        auto bins_x =
            static_cast<int>(output_plots_bins_per_um_ * Units::convert(detector_model_->getPixelSize().x(), "um"));
        auto bins_y =
            static_cast<int>(output_plots_bins_per_um_ * Units::convert(detector_model_->getPixelSize().y(), "um"));
        auto bins_z =
            static_cast<int>(output_plots_bins_per_um_ * Units::convert(detector_model_->getSensorSize().z(), "um"));
        deposition_position_xy =
            CreateHistogram<TH2D>("deposition_position_xy",
                                  "Deposition position, x-y plane;x [#mum];y [#mum]",
                                  bins_x,
                                  -static_cast<double>(Units::convert(detector_model_->getPixelSize().x() / 2, "um")),
                                  static_cast<double>(Units::convert(detector_model_->getPixelSize().x() / 2, "um")),
                                  bins_y,
                                  -static_cast<double>(Units::convert(detector_model_->getPixelSize().y() / 2, "um")),
                                  static_cast<double>(Units::convert(detector_model_->getPixelSize().y() / 2, "um")));
        deposition_position_xz =
            CreateHistogram<TH2D>("deposition_position_xz",
                                  "Deposition position, x-z plane;x [#mum];z [#mum]",
                                  bins_x,
                                  -static_cast<double>(Units::convert(detector_model_->getPixelSize().x() / 2, "um")),
                                  static_cast<double>(Units::convert(detector_model_->getPixelSize().x() / 2, "um")),
                                  bins_z,
                                  -static_cast<double>(Units::convert(detector_model_->getSensorSize().z() / 2, "um")),
                                  static_cast<double>(Units::convert(detector_model_->getSensorSize().z() / 2, "um")));
        deposition_position_yz =
            CreateHistogram<TH2D>("deposition_position_yz",
                                  "Deposition position, y-z plane;y [#mum];z [#mum]",
                                  bins_y,
                                  -static_cast<double>(Units::convert(detector_model_->getPixelSize().y() / 2, "um")),
                                  static_cast<double>(Units::convert(detector_model_->getPixelSize().y() / 2, "um")),
                                  bins_z,
                                  -static_cast<double>(Units::convert(detector_model_->getSensorSize().z() / 2, "um")),
                                  static_cast<double>(Units::convert(detector_model_->getSensorSize().z() / 2, "um")));
    }
}

void DepositionPointChargeModule::run(Event* event) {

    ROOT::Math::XYZPoint position;

    if(model_ == DepositionModel::FIXED) {
        // Fixed position as read from the configuration:
        position = position_;
    } else if(model_ == DepositionModel::SCAN) {
        // Center the volume to be scanned in the center of the sensor,
        // reference point is lower left corner of one pixel volume
        auto ref = position_ + detector_model_->getMatrixSize() / 2.0 + voxel_ / 2.0 -
                   ROOT::Math::XYZVector(detector_model_->getPixelSize().x() / 2.0,
                                         detector_model_->getPixelSize().y() / 2.0,
                                         detector_model_->getSensorSize().z() / 2.0);
        LOG(DEBUG) << "Reference: " << Units::display(ref, {"um", "mm"});
        if(scan_coordinates_.size() == 3) {
            position =
                ROOT::Math::XYZPoint(voxel_.x() * static_cast<double>((event->number - 1) % root_),
                                     voxel_.y() * static_cast<double>(((event->number - 1) / root_) % root_),
                                     voxel_.z() * static_cast<double>(((event->number - 1) / root_ / root_) % root_)) +
                ref;
        } else {
            position = ref;
            if(scan_x_) {
                position.SetX(voxel_.x() * static_cast<double>((event->number - 1) % root_) + ref.x());
                if(scan_y_) {
                    position.SetY(voxel_.y() * static_cast<double>(((event->number - 1) / root_) % root_) + ref.y());
                } else if(scan_z_) {
                    position.SetZ(voxel_.z() * static_cast<double>(((event->number - 1) / root_) % root_) + ref.z());
                }
            } else if(scan_y_) {
                position.SetY(voxel_.y() * static_cast<double>((event->number - 1) % root_) + ref.y());
                if(scan_z_) {
                    position.SetZ(voxel_.z() * static_cast<double>(((event->number - 1) / root_) % root_) + ref.z());
                }
            } else {
                position.SetZ(voxel_.z() * static_cast<double>((event->number - 1) % root_) + ref.z());
            }
        }
        LOG(DEBUG) << "Deposition position in local coordinates: " << Units::display(position, {"um", "mm"});
    } else {
        // Calculate random offset from configured position
        auto shift = [&](auto size) {
            double dx = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
            double dy = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
            double dz = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
            return ROOT::Math::XYZVector(dx, dy, dz);
        };

        // Spot around the configured position
        position = position_ + shift(spot_size_);
    }

    // Create charge carriers at requested position
    if(type_ == SourceType::MIP) {
        DepositLine(event, position);
    } else {
        DepositPoint(event, position);
        if(output_plots_) {
            auto [xpixel, ypixel] = detector_model_->getPixelIndex(position);
            auto inPixelPos = position - detector_model_->getPixelCenter(xpixel, ypixel);
            auto in_pixel_um_x = static_cast<double>(Units::convert(inPixelPos.x(), "um"));
            auto in_pixel_um_y = static_cast<double>(Units::convert(inPixelPos.y(), "um"));
            auto in_pixel_um_z = static_cast<double>(Units::convert(position.z(), "um"));
            deposition_position_xy->Fill(in_pixel_um_x, in_pixel_um_y);
            deposition_position_xz->Fill(in_pixel_um_x, in_pixel_um_z);
            deposition_position_yz->Fill(in_pixel_um_y, in_pixel_um_z);
        }
    }
}

void DepositionPointChargeModule::finalize() {
    if(output_plots_) {
        deposition_position_xy->Get()->SetOption("colz");
        deposition_position_xz->Get()->SetOption("colz");
        deposition_position_yz->Get()->SetOption("colz");

        deposition_position_xy->Write();
        deposition_position_xz->Write();
        deposition_position_yz->Write();
    }
}

void DepositionPointChargeModule::DepositPoint(Event* event, const ROOT::Math::XYZPoint& position) {
    // Vector of deposited charges and their "MCParticle"
    std::vector<DepositedCharge> charges;
    std::vector<MCParticle> mcparticles;

    LOG(DEBUG) << "Position (local coordinates): " << Units::display(position, {"um", "mm"});
    // Cross-check calculated position to be within sensor:
    if(!detector_model_->isWithinSensor(position)) {
        LOG(DEBUG) << "Requested position is outside active sensor volume.";
        return;
    }

    auto position_global = detector_->getGlobalPosition(position);

    // Start and stop position is the same for the MCParticle
    mcparticles.emplace_back(position, position_global, position, position_global, -1, 0., 0.);
    LOG(DEBUG) << "Generated MCParticle at global position " << Units::display(position_global, {"um", "mm"})
               << " in detector " << detector_->getName();
    // Count electrons and holes:
    mcparticles.back().setTotalDepositedCharge(2 * carriers_);

    charges.emplace_back(position, position_global, CarrierType::ELECTRON, carriers_, 0., 0., &(mcparticles.back()));
    charges.emplace_back(position, position_global, CarrierType::HOLE, carriers_, 0., 0., &(mcparticles.back()));
    LOG(DEBUG) << "Deposited " << carriers_ << " charge carriers of both types at global position "
               << Units::display(position_global, {"um", "mm"}) << " in detector " << detector_->getName();

    // Dispatch the messages to the framework
    auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(mcparticles), detector_);
    messenger_->dispatchMessage(this, std::move(mcparticle_message), event);

    auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(charges), detector_);
    messenger_->dispatchMessage(this, std::move(deposit_message), event);
}

void DepositionPointChargeModule::DepositLine(Event* event, const ROOT::Math::XYZPoint& position) {
    // Vector of deposited charges and their "MCParticle"
    std::vector<DepositedCharge> charges;
    std::vector<MCParticle> mcparticles;

    // Cross-check calculated position to be within sensor:
    if(!detector_model_->isWithinSensor(position)) {
        LOG(DEBUG) << "Requested position is outside active sensor volume.";
        return;
    }

    // Start and end position of MCParticle:
    // End point? It's the intersection along the line to the box. Start position is also that, but int he other direction.
    // The given position is a point the line intersects. Need to extrapolate to surfaces using this
    auto [start_local, end_local] = SensorIntersection(position);
    ROOT::Math::XYZPoint start_global = detector_->getGlobalPosition(start_local);
    ROOT::Math::XYZPoint end_global = detector_->getGlobalPosition(end_local);

    // Total number of carriers will be:
    auto charge = static_cast<unsigned int>(carriers_ * (end_local.z() - start_local.z()) / step_size_z_);
    // Create MCParticle:
    mcparticles.emplace_back(start_local, start_global, end_local, end_global, -1, 0., 0.);
    LOG(DEBUG) << "Generated MCParticle with start " << Units::display(start_global, {"um", "mm"}) << " and end "
               << Units::display(end_global, {"um", "mm"}) << " in detector " << detector_->getName();
    // Count electrons and holes:
    mcparticles.back().setTotalDepositedCharge(2 * charge);

    // Deposit the charge carriers:
    auto position_local = start_local;
    while(position_local.z() < end_local.z()) {
        position_local += ROOT::Math::XYZVector(0, 0, step_size_z_);
        auto position_global = detector_->getGlobalPosition(position_local);

        charges.emplace_back(
            position_local, position_global, CarrierType::ELECTRON, carriers_, 0., 0., &(mcparticles.back()));
        charges.emplace_back(position_local, position_global, CarrierType::HOLE, carriers_, 0., 0., &(mcparticles.back()));
        LOG(TRACE) << "Deposited " << carriers_ << " charge carriers of both types at global position "
                   << Units::display(position_global, {"um", "mm"}) << " in detector " << detector_->getName();

        if(output_plots_) {
            auto [xpixel, ypixel] = detector_model_->getPixelIndex(position_local);
            auto inPixelPos = position_local - detector_model_->getPixelCenter(xpixel, ypixel);
            auto in_pixel_um_x = static_cast<double>(Units::convert(inPixelPos.x(), "um"));
            auto in_pixel_um_y = static_cast<double>(Units::convert(inPixelPos.y(), "um"));
            auto in_pixel_um_z = static_cast<double>(Units::convert(position.z(), "um"));
            deposition_position_xy->Fill(in_pixel_um_x, in_pixel_um_y);
            deposition_position_xz->Fill(in_pixel_um_x, in_pixel_um_z);
            deposition_position_yz->Fill(in_pixel_um_y, in_pixel_um_z);
        }
    }

    // Dispatch the messages to the framework
    auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(mcparticles), detector_);
    messenger_->dispatchMessage(this, std::move(mcparticle_message), event);

    auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(charges), detector_);
    messenger_->dispatchMessage(this, std::move(deposit_message), event);
}

std::tuple<ROOT::Math::XYZPoint, ROOT::Math::XYZPoint>
DepositionPointChargeModule::SensorIntersection(const ROOT::Math::XYZPoint& line_origin) {

    double sensorLowerEdges[3] = {-detector_model_->getSensorSize().x() / 2.0,
                                  -detector_model_->getSensorSize().y() / 2.0,
                                  -detector_model_->getSensorSize().z() / 2.0};
    double sensorUpperEdges[3] = {detector_model_->getSensorSize().x() / 2.0,
                                  detector_model_->getSensorSize().y() / 2.0,
                                  detector_model_->getSensorSize().z() / 2.0};
    double lineOriginPoint[3] = {line_origin.x(), line_origin.y(), line_origin.z()};
    double lineDirection[3] = {mip_direction_.x(), mip_direction_.y(), mip_direction_.z()};

    double tMin = -INFINITY;
    double tMax = INFINITY;
    // Loop over the three coordinates
    for(int i = 0; i < 3; i++) {
        double t1 = (sensorLowerEdges[i] - lineOriginPoint[i]) / lineDirection[i];
        double t2 = (sensorUpperEdges[i] - lineOriginPoint[i]) / lineDirection[i];

        tMin = std::max(tMin, std::min(t1, t2));
        tMax = std::min(tMax, std::max(t1, t2));
    }

    ROOT::Math::XYZPoint lowerIntersectPos = line_origin + tMin * mip_direction_;
    ROOT::Math::XYZPoint upperIntersectPos = line_origin + tMax * mip_direction_;
    LOG(WARNING) << "lowerIntersectPos: " << lowerIntersectPos << ", upperIntersectPos: " << upperIntersectPos;
    LOG(WARNING) << "MIP direction: " << mip_direction_;

    return {lowerIntersectPos, upperIntersectPos};
}
