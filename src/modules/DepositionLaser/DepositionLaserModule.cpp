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

#include <Math/AxisAngle.h>
#include <TMath.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>

using namespace allpix;

DepositionLaserModule::DepositionLaserModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), messenger_(messenger) {

    // Read beam parameters from config

    source_position_ = config_.get<ROOT::Math::XYZPoint>("source_position");
    LOG(DEBUG) << "Source position: " << source_position_ << "mm";

    // Make beam_direction a unity vector, so t-values produced by clipping algorithm are in actual length units
    beam_direction_ = config_.get<ROOT::Math::XYZVector>("beam_direction").Unit();
    LOG(DEBUG) << "Beam direction: " << beam_direction_;

    config_.setDefault<double>("beam_waist", 0.02);
    config_.setDefault<int>("number_of_photons", 10000);

    size_t convergence_params_count = config_.count({"focal_distance", "beam_convergence"});
    switch(convergence_params_count) {
    case 0:
        LOG(DEBUG) << "Beam geometry: cylindrical";
        break;
    case 1:
        throw InvalidCombinationError(config_,
                                      {"focal_distance", "beam_convergence"},
                                      "Both focal distance and convergence should be given for a gaussian beam");
        break;
    case 2:
        LOG(DEBUG) << "Beam geometry: converging";
        focal_distance_ = config_.get<double>("focal_distance");
        beam_convergence_ = config_.get<double>("beam_convergence");
        LOG(DEBUG) << "Focal distance: " << Units::display(focal_distance_.value(), "mm")
                   << ", convergence angle: " << Units::display(beam_convergence_.value(), "deg");
        break;
    default:
        break;
    }

    beam_waist_ = config_.get<double>("beam_waist");
    LOG(DEBUG) << "Beam waist: " << Units::display(beam_waist_, "um");
    if(beam_waist_ < 0) {
        throw InvalidValueError(config_, "beam_waist", "Beam waist should be a positive value!");
    }

    number_of_photons_ = config_.get<size_t>("number_of_photons");
    LOG(DEBUG) << "Number of photons: " << number_of_photons_;
    if(number_of_photons_ == 0) {
        throw InvalidValueError(config_, "number_of_photons", "Number of photons should be a nonzero value!");
    }

    config_.setDefault<double>("pulse_duration", 0.5);
    pulse_duration_ = config_.get<double>("pulse_duration");
    LOG(DEBUG) << "Pulse duration: " << Units::display(pulse_duration_, "ns");
    if(pulse_duration_ < 0) {
        throw InvalidValueError(config_, "pulse_duration_", "Pulse should be a positive value!");
    }

    config_.setDefault<bool>("verbose_tracking", false);
    verbose_tracking_ = config_.get<bool>("verbose_tracking");

    // FIXME less hardcoded values
    wavelength_ = config_.get<double>("wavelength");
    if(Units::convert(wavelength_, "nm") < 250 || Units::convert(wavelength_, "nm") > 1450) {
        throw InvalidValueError(config_, "wavelength", "Currently supported wavelengths are 250 -- 1450 nm");
    }

    config_.setDefault<bool>("output_plots", false);
    output_plots_ = config.get<bool>("output_plots");
}

void DepositionLaserModule::initialize() {

    // Load data
    std::string laser_data_path = ALLPIX_LASER_DATA_DIRECTORY;

    std::ifstream f(std::filesystem::path(laser_data_path) / "silicon_photoabsorption.data");

    std::map<double, double> absorption_lut;
    for(auto it = std::istream_iterator<double>(f); it != std::istream_iterator<double>(); it++) {
        double wavelength_nm = *it;
        double abs_length_mm = *(++it);
        absorption_lut[wavelength_nm] = abs_length_mm;
    }

    LOG(DEBUG) << "Loading absorption data: " << laser_data_path;

    // Find or interpolate absorption depth for given wavelength
    double wavelength_nm = Units::convert(wavelength_, "nm");
    if(absorption_lut.count(wavelength_nm) != 0) {
        absorption_length_ = absorption_lut[wavelength_nm];
    } else {
        auto it = absorption_lut.upper_bound(wavelength_nm);
        double wl1 = (*prev(it)).first;
        double wl2 = (*it).first;
        absorption_length_ =
            (absorption_lut[wl1] * (wl2 - wavelength_) + absorption_lut[wl2] * (wavelength_ - wl1)) / (wl2 - wl1);
    }

    LOG(DEBUG) << "Wavelength = " << Units::display(wavelength_, "nm") << ", corresponding absorption length is "
               << Units::display(absorption_length_, "um");

    // Create Histograms
    LOG(DEBUG) << "Initializing histograms";
    if(output_plots_) {
        Int_t nbins = 100;
        double nsigmas = 3;
        double focalplane_histsize = beam_waist_ * nsigmas;

        h_intensity_focalplane_ = CreateHistogram<TH2D>("intensity_focalplane",
                                                        "Beam profile in focal plane, a.u.",
                                                        nbins,
                                                        -focalplane_histsize,
                                                        focalplane_histsize,
                                                        nbins,
                                                        -focalplane_histsize,
                                                        focalplane_histsize);

        double sourceplane_histsize = focalplane_histsize;
        if(beam_convergence_ && focal_distance_) {
            sourceplane_histsize += focal_distance_.value() * sin(beam_convergence_.value());
        }

        h_intensity_sourceplane_ = CreateHistogram<TH2D>("intensity_sourceplane",
                                                         "Beam profile at source, a.u.",
                                                         nbins,
                                                         -sourceplane_histsize,
                                                         sourceplane_histsize,
                                                         nbins,
                                                         -sourceplane_histsize,
                                                         sourceplane_histsize);

        h_angular_phi_ =
            CreateHistogram<TH1D>("phi_distribution", "Phi_distribution w.r.t. beam direction", nbins, -3.5, 3.5);
        h_angular_theta_ =
            CreateHistogram<TH1D>("theta_distribution", "Theta distribution w.r.t. beam direction", nbins, 0, 45);
    }
}

void DepositionLaserModule::run(Event* event) {

    // Lambda to generate two unit vectors, orthogonal to beam direction
    // Adapted from TVector3::Orthogonal()
    auto orthogonal_pair = [](const ROOT::Math::XYZVector& v) {
        // Additional convenience variables for components' absolute values
        double abs_x = v.X() < 0.0 ? -v.X() : v.X();
        double abs_y = v.Y() < 0.0 ? -v.Y() : v.Y();
        double abs_z = v.Z() < 0.0 ? -v.Z() : v.Z();

        ROOT::Math::XYZVector v1, v2;

        if(abs_x < abs_y) {
            v1 = (abs_x < abs_z ? ROOT::Math::XYZVector(0, v.Z(), -v.Y()) : ROOT::Math::XYZVector(v.Y(), -v.X(), 0));
        } else {
            v1 = (abs_y < abs_z ? ROOT::Math::XYZVector(-v.Z(), 0, v.X()) : ROOT::Math::XYZVector(v.Y(), -v.X(), 0));
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

    // Lambda to get scalar orthogonal components w.r.t. beam_direction
    auto orthogonal_components = [&](const ROOT::Math::XYZVector& v) {
        auto [v1, v2] = orthogonal_pair(beam_direction_);
        return std::make_pair(v.Dot(v1), v.Dot(v2));
    };

    // Containers for output messages

    std::map<std::shared_ptr<Detector>, std::vector<MCParticle>> mc_particles;
    std::map<std::shared_ptr<Detector>, std::vector<DepositedCharge>> deposited_charges;

    // Containers for timestamps
    // Starting time points are generated in advance to correctly shift zero afterwards
    double c = TMath::C() * 100; // speed of light in mm/ns
    std::vector<double> starting_times(number_of_photons_);
    std::for_each(begin(starting_times), end(starting_times), [&](auto& item) {
        item = allpix::normal_distribution<double>(0, pulse_duration_)(event->getRandomEngine());
    });

    std::sort(begin(starting_times), end(starting_times));
    double starting_time_offset = starting_times[0];
    std::for_each(
        begin(starting_times), end(starting_times), [starting_time_offset](auto& item) { item -= starting_time_offset; });

    // To correctly offset local time for each detector
    std::map<std::shared_ptr<Detector>, double> local_time_offsets;

    // Loop over photons in a single laser pulse
    // In time order
    for(size_t i_photon = 0; i_photon < number_of_photons_; ++i_photon) {

        std::string event_name = std::to_string(event->number);
        LOG_PROGRESS(INFO, event_name) << "Event " << event_name << ": photon " << i_photon + 1 << " of "
                                       << number_of_photons_;

        // Starting point and direction for this exact photon
        ROOT::Math::XYZPoint starting_point{};
        ROOT::Math::XYZVector photon_direction{};

        // Generate starting point and direction in the beam
        if(beam_convergence_ && focal_distance_) {
            // Converging beam case

            // Generate correct position in the focal plane
            auto focal_position =
                source_position_ + beam_direction_ * focal_distance_.value() + beam_pos_smearing(beam_waist_);

            // Generate angles
            double phi = allpix::uniform_real_distribution<double>(0, 2 * TMath::Pi())(event->getRandomEngine());
            double cos_theta =
                allpix::uniform_real_distribution<double>(cos(beam_convergence_.value()), 1)(event->getRandomEngine());

            // Rotate direction by given angles
            // First, define and apply theta rotation
            ROOT::Math::XYZVector theta_axis = orthogonal_pair(beam_direction_).first;
            ROOT::Math::AxisAngle theta_rotation(theta_axis, acos(cos_theta));
            photon_direction = theta_rotation(beam_direction_);

            // Second, rotate that around the beam axis
            ROOT::Math::AxisAngle phi_rotation(beam_direction_, phi);
            photon_direction = phi_rotation(photon_direction);

            // Backtrack position from the focal plane to the source plane
            // This calculation is nuts, will add an explanation somewhere
            starting_point = focal_position + photon_direction * beam_direction_.Dot(source_position_ - focal_position) /
                                                  beam_direction_.Dot(photon_direction);

            if(output_plots_) {
                // Fill beam profile histos
                auto [dx_focal, dy_focal] = orthogonal_components(focal_position - source_position_);
                h_intensity_focalplane_->Fill(dx_focal, dy_focal);
                auto [dx_source, dy_source] = orthogonal_components(starting_point - source_position_);
                h_intensity_sourceplane_->Fill(dx_source, dy_source);
            }

        } else {
            // Cylindrical beam case
            starting_point = source_position_ + beam_pos_smearing(beam_waist_);
            photon_direction = beam_direction_;

            if(output_plots_) {
                // Fill beam profle histos
                auto [dx_source, dy_source] = orthogonal_components(starting_point - source_position_);
                h_intensity_sourceplane_->Fill(dx_source, dy_source);
                h_intensity_focalplane_->Fill(dx_source, dy_source);
            }
        }

        // Fill histograms if needed
        if(output_plots_) {
            // Both are unit vectors
            double theta = static_cast<double>(Units::convert(acos(beam_direction_.Dot(photon_direction)), "deg"));
            auto [dx, dy] = orthogonal_components(photon_direction);
            double phi = atan2(dy, dx);
            h_angular_phi_->Fill(phi);
            h_angular_theta_->Fill(theta);
        }

        LOG(DEBUG) << "    Starting point: " << starting_point << "mm, direction: " << photon_direction;
        ;

        // Get starting time in the pulse
        double starting_time = starting_times[i_photon];
        LOG(DEBUG) << "    Starting timestamp: " << Units::display(starting_time, "ns");

        // Generate penetration depth
        double penetration_depth =
            allpix::exponential_distribution<double>(1 / absorption_length_)(event->getRandomEngine());
        LOG(DEBUG) << "    Penetration depth: " << Units::display(penetration_depth, "um");

        // Check intersections with every detector
        std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
        std::vector<std::pair<std::shared_ptr<Detector>, std::pair<double, double>>> intersection_segments;

        for(auto& detector : detectors) {
            auto intersection = get_intersection(detector, starting_point, photon_direction);
            if(intersection) {
                intersection_segments.emplace_back(std::make_pair(detector, intersection.value()));
            } else if(verbose_tracking_) {
                LOG(DEBUG) << "    Intersection with " << detector->getName() << ": no intersection";
            }
        }

        // Sort intersection segments along the track, starting from closest to source
        // Since beam_direction is a unity vector, t-values produced by clipping algorithm are in actual length units

        std::sort(begin(intersection_segments), end(intersection_segments), [](const auto& p1, const auto& p2) {
            return p1.second < p2.second;
        });

        bool hit = false;
        double t_hit = 0;
        double t0_hit = 0;
        std::shared_ptr<Detector> d_hit;

        for(const auto& [detector, points] : intersection_segments) {
            double t0 = points.first;
            double t1 = points.second;
            double distance = t1 - t0;

            ROOT::Math::XYZPoint entry_point = starting_point + photon_direction * t0;
            ROOT::Math::XYZPoint exit_point = starting_point + photon_direction * t1;
            if(verbose_tracking_) {
                LOG(DEBUG) << "    Intersection with " << detector->getName() << ": travel distance "
                           << Units::display(distance, "um");
                LOG(DEBUG) << "        entry at " << entry_point << "mm, " << Units::display(starting_time + t0 / c, "ns");
                LOG(DEBUG) << "        exit at " << exit_point << "mm, " << Units::display(starting_time + t1 / c, "ns");
            }

            // Check for a hit
            if(penetration_depth < distance && !hit) {
                hit = true;
                t_hit = t0 + penetration_depth;
                t0_hit = t0;
                d_hit = detector;
                if(!verbose_tracking_) {
                    break;
                }
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
            auto hit_entry_global = starting_point + photon_direction * t0_hit;
            auto hit_global = starting_point + photon_direction * t_hit;
            auto hit_entry_local = d_hit->getLocalPosition(hit_entry_global);
            auto hit_local = d_hit->getLocalPosition(hit_global);

            double time_entry_global = starting_time + t0_hit / c;
            double time_hit_global = starting_time + t_hit / c;
            double time_entry_local = time_entry_global - local_time_offsets[d_hit];
            double time_hit_local = time_hit_global - local_time_offsets[d_hit];

            LOG(DEBUG) << "    Hit in " << d_hit->getName();
            LOG(DEBUG) << "        global: " << hit_global << "mm, " << Units::display(time_hit_global, "ns");
            LOG(DEBUG) << "        local: " << hit_local << "mm, " << Units::display(time_hit_local, "ns");

            // If that is a first hit in this detector, create map entries
            if(mc_particles.count(d_hit) == 0) {
                // This vector is initially assigned size
                // Otherwise, resize() would be called  during its fill
                // and this will break pointers to MCParticle's in DepositedCharge's
                mc_particles[d_hit] = std::vector<MCParticle>();
                mc_particles[d_hit].reserve(number_of_photons_);
            }
            if(deposited_charges.count(d_hit) == 0) {
                deposited_charges[d_hit] = std::vector<DepositedCharge>();
            }

            // Construct all necessary objects in-place
            // allpix::MCParticle
            mc_particles[d_hit].emplace_back(hit_entry_local,
                                             hit_entry_global,
                                             hit_local,
                                             hit_global,
                                             22, // gamma
                                             time_entry_local,
                                             time_entry_global);

            // allpix::DepositedCharge for electron
            deposited_charges[d_hit].emplace_back(hit_local,
                                                  hit_global,
                                                  CarrierType::ELECTRON,
                                                  1, // value
                                                  time_hit_local,
                                                  time_hit_global,
                                                  &(mc_particles[d_hit].back()));

            // allpix::DepositedCharge for hole
            deposited_charges[d_hit].emplace_back(hit_local,
                                                  hit_global,
                                                  CarrierType::HOLE,
                                                  1, // value
                                                  time_hit_local,
                                                  time_hit_global,
                                                  &(mc_particles[d_hit].back()));
        }
    } // loop over photons

    LOG(DEBUG) << "Registered hits in " << mc_particles.size() << " detectors";
    // Dispatch messages
    for(auto& [detector, data] : mc_particles) {
        LOG(DEBUG) << detector->getName() << ": " << data.size() << " hits";
        auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(data), detector);
        messenger_->dispatchMessage(this, mcparticle_message, event);
    }

    for(auto& [detector, data] : deposited_charges) {
        auto charge_message = std::make_shared<DepositedChargeMessage>(std::move(data), detector);
        messenger_->dispatchMessage(this, charge_message, event);
    }
}

void DepositionLaserModule::finalize() {
    if(output_plots_) {
        h_intensity_focalplane_->Write();
        h_intensity_sourceplane_->Write();
        h_angular_phi_->Write();
        h_angular_theta_->Write();
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
