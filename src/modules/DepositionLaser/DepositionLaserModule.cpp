/**
 * @file
 * @brief Implementation of [DepositionLaser] module
 *
 * @copyright Copyright (c) 2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <DepositionLaserModule.hpp>

#include <core/utils/distributions.h>
#include <core/utils/log.h>
#include <objects/DepositedCharge.hpp>
#include <objects/MCParticle.hpp>

#include <TMath.h>

#include <algorithm>

using namespace allpix;

DepositionLaserModule::DepositionLaserModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), messenger_(messenger) {

    // Read beam parameters from config

    if(config_.has("source_position")) {
        source_position_ = config_.get<ROOT::Math::XYZPoint>("source_position");
        LOG(DEBUG) << "Source position: " << source_position_;
    } else {
        throw MissingKeyError("source_position", "DepositionLaser");
    }

    if(config_.has("beam_direction")) {
        // Make beam_direction a unity vector, so t-values produced by clipping algorithm are in actual length units
        beam_direction_ = config_.get<ROOT::Math::XYZVector>("beam_direction").Unit();
        LOG(DEBUG) << "Beam direction: " << beam_direction_;
    } else {
        throw MissingKeyError("beam_direction", "DepositionLaser");
    }

    config_.setDefault<double>("beam_waist", 0.02);
    config_.setDefault<int>("photon_number", 10000);

    beam_waist_ = config_.get<double>("beam_waist");
    LOG(DEBUG) << "Beam waist: " << Units::convert(beam_waist_, "um") << " um";
    if(beam_waist_ < 0) {
        throw InvalidValueError(config_, "beam_waist", "Beam waist should be a positive value!");
    }

    photon_number_ = config_.get<size_t>("photon_number");
    LOG(DEBUG) << "Photon number: " << photon_number_;
    if(photon_number_ <= 0) {
        throw InvalidValueError(config_, "photon_number", "Photon number should be a positive nonzero value!");
    }
}

void DepositionLaserModule::initialize() {

    // Load data here maybe...
}

void DepositionLaserModule::run(Event* event) {

    // Lambda to generate two unit vectors, orthogonal to beam direction
    // Adapted from TVector3::Orthogonal()
    auto orthogonal_pair = [](const ROOT::Math::XYZVector& v) {
        double xx = v.X() < 0.0 ? -v.X() : v.X();
        double yy = v.Y() < 0.0 ? -v.Y() : v.Y();
        double zz = v.Z() < 0.0 ? -v.Z() : v.Z();

        ROOT::Math::XYZVector v1, v2;

        if(xx < yy) {
            if(xx < zz) {
                v1 = ROOT::Math::XYZVector(0, v.Z(), -v.Y());
            } else {
                v1 = ROOT::Math::XYZVector(v.Y(), -v.X(), 0);
            }
        } else {
            if(yy < zz) {
                v1 = ROOT::Math::XYZVector(-v.Z(), 0, v.X());
            } else {
                v1 = ROOT::Math::XYZVector(v.Y(), -v.X(), 0);
            }
        }

        v2 = v.Cross(v1);
        return std::make_pair(v1.Unit(), v2.Unit());
    };

    // Lambda to generate a smearing vector
    auto beam_pos_smearing = [&](auto size) {
        auto [v1, v2] = orthogonal_pair(beam_direction_);
        double dx = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
        double dy = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
        return v1 * dx + v2 * dy;
    };

    // Containers for output messages

    std::map<std::shared_ptr<Detector>, std::vector<MCParticle>> mc_particles;
    std::map<std::shared_ptr<Detector>, std::vector<DepositedCharge>> deposited_charges;

    // Containers for timestamps
    // Starting time points are generated in advance to correctly shift zero afterwards
    // FIXME Read pulse duration from the config instead
    double c = 299.792;              // mm/ns
    double laser_pulse_duration = 1; // ns
    std::vector<double> starting_times(photon_number_);
    for(auto& item : starting_times) {
        item = allpix::normal_distribution<double>(0, laser_pulse_duration)(event->getRandomEngine());
    }

    std::sort(begin(starting_times), end(starting_times));
    double starting_time_offset = starting_times[0];
    for(auto& item : starting_times) {
        item -= starting_time_offset;
    }

    // To correctly offset local time for each detector
    std::map<std::shared_ptr<Detector>, double> local_time_offsets;

    // Loop over photons in a single laser pulse
    // In time order
    for(size_t i_photon = 0; i_photon < photon_number_; ++i_photon) {

        std::string event_name = std::to_string(event->number);
        LOG_PROGRESS(INFO, event_name) << "Event " << event_name << ": photon " << i_photon + 1 << " of " << photon_number_;

        // Generate starting point in the beam
        ROOT::Math::XYZPoint starting_point = source_position_ + beam_pos_smearing(beam_waist_);
        LOG(DEBUG) << "    Starting point: " << starting_point << ", direction " << beam_direction_;

        // Get starting time in the pulse
        double starting_time = starting_times[i_photon];
        LOG(DEBUG) << "    Starting timestamp: " << starting_time << " ns";

        // Generate penetration depth
        // FIXME Load absorption length from data instead
        double absorption_length = 0.5; // mm
        double penetration_depth = allpix::exponential_distribution<double>(1 / absorption_length)(event->getRandomEngine());
        LOG(DEBUG) << "    Penetration depth: " << Units::convert(penetration_depth, "um") << " um";

        // Check intersections with every detector
        std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
        std::vector<std::pair<std::shared_ptr<Detector>, std::pair<double, double>>> intersection_segments;

        for(auto& detector : detectors) {
            auto intersection = get_intersection(detector, starting_point, beam_direction_);
            if(intersection) {
                intersection_segments.emplace_back(std::make_pair(detector, intersection.value()));
            } else {
                LOG(DEBUG) << "    Intersection with " << detector->getName() << ": no intersection";
            }
        }

        // Sort intersection segments along the track, starting from closest to source
        // Since beam_direction is a unity vector, t-values produced by clipping algorithm are in actual length units

        auto comp = [](const auto& p1, const auto& p2) { return p1.second < p2.second; };
        std::sort(begin(intersection_segments), end(intersection_segments), comp);

        bool hit = false;
        double t_hit = 0;
        double t0_hit = 0;
        std::shared_ptr<Detector> d_hit;

        for(const auto& [detector, points] : intersection_segments) {
            double t0 = points.first;
            double t1 = points.second;
            double distance = t1 - t0;

            ROOT::Math::XYZPoint entry_point = starting_point + beam_direction_ * t0;
            ROOT::Math::XYZPoint exit_point = starting_point + beam_direction_ * t1;
            LOG(DEBUG) << "    Intersection with " << detector->getName() << ": travel distance "
                       << Units::convert(distance, "um") << " um, ";
            LOG(DEBUG) << "        entry at " << entry_point << " mm, " << starting_time + t0 / c << " ns";
            LOG(DEBUG) << "        exit at " << exit_point << " mm, " << starting_time + t1 / c << " ns";

            // Check for a hit
            if(penetration_depth < distance && !hit) {
                hit = true;
                t_hit = t0 + penetration_depth;
                t0_hit = t0;
                d_hit = detector;
                // Go till the end of this loop anyway to produce consistent log output
            } else {
                penetration_depth -= distance;
            }
        }

        if(hit) {
            // If this was the first hit in this detector in this event, remember entry timestamp as local t=0 for this
            // detector Here I boldly assume that photon that is created earlier also hits earlier
            if(local_time_offsets.count(d_hit) == 0) {
                local_time_offsets[d_hit] = starting_time + t0_hit / c;
            }

            // Create and store corresponding MCParticle and DepositedCharge
            auto hit_entry_global = starting_point + beam_direction_ * t0_hit;
            auto hit_global = starting_point + beam_direction_ * t_hit;
            auto hit_entry_local = d_hit->getLocalPosition(hit_entry_global);
            auto hit_local = d_hit->getLocalPosition(hit_global);

            double time_entry_global = starting_time + t0_hit / c;
            double time_hit_global = starting_time + t_hit / c;
            double time_entry_local = time_entry_global - local_time_offsets[d_hit];
            double time_hit_local = time_hit_global - local_time_offsets[d_hit];

            LOG(DEBUG) << "    Hit in " << d_hit->getName();
            LOG(DEBUG) << "        global: " << hit_global << " mm, " << time_hit_global << " ns";
            LOG(DEBUG) << "        local: " << hit_local << " mm, " << time_hit_local << " ns";

            MCParticle p(hit_entry_local,
                         hit_entry_global,
                         hit_local,
                         hit_global,
                         22, // gamma
                         time_entry_local,
                         time_entry_global);

            // setCarrierType method does not exist so that object is created twice :(
            DepositedCharge d_e(hit_local,
                                hit_global,
                                CarrierType::ELECTRON,
                                1, // value
                                time_hit_local,
                                time_hit_global);

            DepositedCharge d_h(hit_local,
                                hit_global,
                                CarrierType::HOLE,
                                1, // value
                                time_hit_local,
                                time_hit_global);

            if(mc_particles.count(d_hit) == 0) {
                mc_particles[d_hit] = std::vector<MCParticle>();
                deposited_charges[d_hit] = std::vector<DepositedCharge>();
            }
            mc_particles[d_hit].push_back(p);
            deposited_charges[d_hit].push_back(d_e);
            deposited_charges[d_hit].push_back(d_h);
        }
    } // loop over photons

    LOG(INFO) << "Registered hits in " << mc_particles.size() << " detectors";
    // Dispatch messages
    for(auto& [detector, data] : mc_particles) {
        LOG(INFO) << detector->getName() << ": " << data.size() << " hits";

        // I am not sure whether move works correctly in this case
        auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(data), detector);
        messenger_->dispatchMessage(this, mcparticle_message, event);

        auto charge_message = std::make_shared<DepositedChargeMessage>(std::move(deposited_charges[detector]), detector);
        messenger_->dispatchMessage(this, charge_message, event);
    }
}

std::optional<std::pair<double, double>>
DepositionLaserModule::get_intersection(const std::shared_ptr<const Detector>& detector,
                                        const ROOT::Math::XYZPoint& position_global,
                                        const ROOT::Math::XYZVector& direction_global) const {
    // Obtain total sensor size
    auto sensor = detector->getModel()->getSensorSize();

    // Transformation from locally centered into global coordinate system, consisting of
    // * The rotation into the global coordinate system
    // * The shift from the origin to the detector position
    // Sensor-centered coordinate system is required for proper clipping!
    ROOT::Math::Rotation3D rotation_center(detector->getOrientation());
    ROOT::Math::Translation3D translation_center(static_cast<ROOT::Math::XYZVector>(detector->getPosition()));
    ROOT::Math::Transform3D transform_center(rotation_center, translation_center);
    auto position = transform_center.Inverse()(position_global);

    // Direction vector can directly be rotated
    auto direction_local = detector->getOrientation().Inverse()(direction_global);

    // Liang–Barsky clipping of a line against faces of a box
    auto clip = [](double denominator, double numerator, double& t0, double& t1) {
        if(denominator > 0) {
            if(numerator > denominator * t1) {
                return false;
            }
            if(numerator > denominator * t0) {
                t0 = numerator / denominator;
            }
            return true;
        } else if(denominator < 0) {
            if(numerator > denominator * t0) {
                return false;
            }
            if(numerator > denominator * t1) {
                t1 = numerator / denominator;
            }
            return true;
        } else {
            return numerator <= 0;
        }
    };

    // Clip the particle track against the six possible box faces
    double t0 = std::numeric_limits<double>::lowest(), t1 = std::numeric_limits<double>::max();
    bool intersect = clip(direction_local.X(), -position.X() - sensor.X() / 2, t0, t1) &&
                     clip(-direction_local.X(), position.X() - sensor.X() / 2, t0, t1) &&
                     clip(direction_local.Y(), -position.Y() - sensor.Y() / 2, t0, t1) &&
                     clip(-direction_local.Y(), position.Y() - sensor.Y() / 2, t0, t1) &&
                     clip(direction_local.Z(), -position.Z() - sensor.Z() / 2, t0, t1) &&
                     clip(-direction_local.Z(), position.Z() - sensor.Z() / 2, t0, t1);

    // The intersection is a point P + t * D with t = t0. Return if positive (i.e. in direction of track vector)
    if(intersect && t0 > 0) {
        // Return distance to entry and exit points
        return std::make_pair(t0, t1);
    } else {
        // Otherwise: The line does not intersect the box.
        return std::nullopt;
    }
}
