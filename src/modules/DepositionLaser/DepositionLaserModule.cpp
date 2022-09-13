/**
 * @file
 * @brief Implementation of [DepositionLaser] module
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DepositionLaserModule.hpp"

#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "objects/MCParticle.hpp"

#include "boost/random/exponential_distribution.hpp"

#include "TMath.h"

#include <algorithm>

using namespace allpix;

DepositionLaserModule::DepositionLaserModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), messenger_(messenger) {

    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    // Input required by this module

    // Read beam parameters from config

    if(config_.has("source_position")) {
        source_position_ = config_.get<ROOT::Math::XYZPoint>("source_position");
        LOG(DEBUG) << "Source position: " << source_position_;
    } else {
        throw MissingKeyError("source_position", "DepositionLaser");
    }

    if(config_.has("beam_direction")) {
        beam_direction_ = config_.get<ROOT::Math::XYZVector>("beam_direction").Unit();
        LOG(DEBUG) << "Beam direction: " << beam_direction_;
    } else {
        throw MissingKeyError("beam_direction", "DepositionLaser");
    }

    config_.setDefault<double>("beam_waist", 0.02);
    config_.setDefault<int>("photon_number", 1000);

    beam_waist_ = config_.get<double>("beam_waist");
    LOG(DEBUG) << "Beam waist: " << Units::convert(beam_waist_, "um") << " um";
    if(beam_waist_ < 0) {
        throw InvalidValueError(config_, "beam_waist", "Beam waist should be a positive value!");
    }

    photon_number_ = config_.get<int>("photon_number");
    LOG(DEBUG) << "Photon number: " << photon_number_;
    if(photon_number_ <= 0) {
        throw InvalidValueError(config_, "photon_number", "Photon number should be a positive nonzero value!");
    }
}

void DepositionLaserModule::initialize() {

    // Loop over detectors and do something
    std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
    for(auto& detector : detectors) {
        // Get the detector name
        std::string detectorName = detector->getName();
        LOG(DEBUG) << "Detector with name " << detectorName;
    }
}

void DepositionLaserModule::run(Event* event) {

    // Lambda for generating two unit vectors, orthogonal to beam direction:
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
        return std::make_pair<ROOT::Math::XYZVector>(v1.Unit(), v2.Unit());
    };

    // Lambda for smearing the initial particle position with the beam size
    auto beam_pos_smearing = [&](auto size) {
        auto [v1, v2] = orthogonal_pair(beam_direction_);
        double dx = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
        double dy = allpix::normal_distribution<double>(0, size)(event->getRandomEngine());
        return v1 * dx + v2 * dy;
    };

    // Loop over photons in a single laser pulse
    for(int i_photon = 0; i_photon < photon_number_; ++i_photon) {

        // Generate starting point in the beam
        ROOT::Math::XYZPoint starting_point = source_position_ + beam_pos_smearing(beam_waist_);
        LOG(DEBUG) << "Photon " << i_photon + 1 << " of " << photon_number_;
        LOG(DEBUG) << "Generated starting point: " << starting_point;

        // Generate starting time in the pulse
        // FIXME Read pulse duration from the config
        double time_smear = 1; // ns
        double starting_time = allpix::normal_distribution<double>(0, time_smear)(event->getRandomEngine());

        // Generate penetration depth
        // FIXME Use actual pen depth
        double absorption_length = 0.5; // mm
        double penetration_depth =
            boost::random::exponential_distribution<double>(1 / absorption_length)(event->getRandomEngine());
        LOG(DEBUG) << "Generated penetration depth: " << Units::convert(penetration_depth, "um");

        // Check intersections with every detector
        std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
        std::vector<std::pair<std::shared_ptr<Detector>, std::pair<double, double>>> intersection_segments;

        for(auto& detector : detectors) {
            auto intersection = get_intersection(detector, starting_point, beam_direction_);
            if(intersection) {
                intersection_segments.push_back(std::make_pair(detector, intersection.value()));
            } else {
                LOG(DEBUG) << detector->getName() << ": no intersection";
            }
        }

        // Sort intersection segments along the track, starting from closest to source

        auto comp = []<typename T>(const T& p1, const T& p2) { return p1.second < p2.second; };
        std::sort(begin(intersection_segments), end(intersection_segments), comp);

        bool hit = false;
        double t_hit, t0_hit;
        std::shared_ptr<Detector> d_hit;
        for(const auto& [detector, points] : intersection_segments) {
            double t0 = points.first;
            double t1 = points.second;
            double distance = t1 - t0;

            ROOT::Math::XYZPoint entry_point = starting_point + beam_direction_ * t0;
            ROOT::Math::XYZPoint exit_point = starting_point + beam_direction_ * t1;
            LOG(DEBUG) << detector->getName() << ": travel distance " << Units::convert(distance, "um") << " um, entry at "
                       << std::setprecision(5) << entry_point << ", exit at " << exit_point;

            // Check for a hit
            if(penetration_depth > distance) {
                penetration_depth -= distance;
            } else {
                if(!hit) {
                    hit = true;
                    t_hit = t0 + penetration_depth;
                    t0_hit = t0;
                    d_hit = detector;
                }
            }
        }

        if(hit) {
            ROOT::Math::XYZPoint hit_global = starting_point + beam_direction_ * t_hit;
            LOG(DEBUG) << "Hit in " << d_hit->getName() << " at " << hit_global;
        }
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
