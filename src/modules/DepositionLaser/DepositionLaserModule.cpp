/**
 * @file
 * @brief Implementation of [DepositionLaser] module
 *
 * @copyright Copyright (c) 2022-2023 CERN and the Allpix Squared authors.
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
#include <tools/liang_barsky.h>

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

    allow_multithreading();

    //
    // Read beam parameters from config
    //

    source_position_ = config_.get<ROOT::Math::XYZPoint>("source_position");
    LOG(DEBUG) << "Source position: " << Units::display(source_position_, {"mm"});

    // Make beam_direction a unity vector, so t-values produced by clipping algorithm are in actual length units
    beam_direction_ = config_.get<ROOT::Math::XYZVector>("beam_direction").Unit();
    LOG(DEBUG) << "Beam direction: " << beam_direction_;

    beam_geometry_ = config_.get<BeamGeometry>("beam_geometry");
    size_t convergence_params_count = config_.count({"focal_distance", "beam_convergence_angle"});
    if(beam_geometry_ == BeamGeometry::CYLINDRICAL) {
        LOG(DEBUG) << "Beam geometry: cylindrical";
        if(convergence_params_count > 0) {
            LOG(DEBUG) << "Beam convergence parameters are ignored for a cylindrical beam";
        }
    } else if(beam_geometry_ == BeamGeometry::CONVERGING) {
        LOG(DEBUG) << "Beam geometry: converging";
        if(convergence_params_count < 2) {
            throw InvalidCombinationError(config_,
                                          {"beam_geometry", "focal_distance", "beam_convergence_angle"},
                                          "Both focal distance and convergence should be specified for a gaussian beam");
        }
        focal_distance_ = config_.get<double>("focal_distance");
        beam_convergence_angle_ = config_.get<double>("beam_convergence_angle");
        LOG(DEBUG) << "Focal distance: " << Units::display(focal_distance_, "mm")
                   << ", convergence angle: " << Units::display(beam_convergence_angle_, "deg");
    }

    config_.setDefault<double>("beam_waist", 0.02);
    beam_waist_ = config_.get<double>("beam_waist");
    LOG(DEBUG) << "Beam waist: " << Units::display(beam_waist_, "um");
    if(beam_waist_ < 0) {
        throw InvalidValueError(config_, "beam_waist", "Beam waist should be a positive value");
    }

    config_.setDefault<int>("number_of_photons", 10000);
    number_of_photons_ = config_.get<size_t>("number_of_photons");
    LOG(DEBUG) << "Number of photons: " << number_of_photons_;
    if(number_of_photons_ == 0) {
        throw InvalidValueError(config_, "number_of_photons", "Number of photons should be a nonzero value");
    }

    config_.setDefault<int>("group_photons", 1);
    group_photons_ = config_.get<size_t>("group_photons");
    if(group_photons_ == 0) {
        throw InvalidValueError(config_, "group_photons", "Should be a nonzero value");
    } else if(group_photons_ > 1) {
        number_of_photons_ /= group_photons_;
        LOG(DEBUG) << "Photons will be generated as " << number_of_photons_ << " groups of " << group_photons_;
    }

    config_.setDefault<double>("pulse_duration", 0.5);
    pulse_duration_ = config_.get<double>("pulse_duration");
    LOG(DEBUG) << "Pulse duration: " << Units::display(pulse_duration_, "ns");
    if(pulse_duration_ < 0) {
        throw InvalidValueError(config_, "pulse_duration_", "Pulse should be a positive value");
    }

    // Select user optics or silicon absorption lookup:
    is_user_optics_ = (config_.count({"absorption_length", "refractive_index"}) == 2);

    if(config_.count({"absorption_length", "refractive_index", "wavelength"}) == 3) {
        throw InvalidCombinationError(config_,
                                      {"absorption_length", "refractive_index", "wavelength"},
                                      "User definition for optical parameters and wavelength are mutually exclusive!");
    }

    if(is_user_optics_) {
        absorption_length_ = config_.get<double>("absorption_length");
        refractive_index_ = config_.get<double>("refractive_index");
        LOG(DEBUG) << "Setting user-defined optical properties for sensor material";
    } else {
        wavelength_ = config_.get<double>("wavelength");
        if(Units::convert(wavelength_, "nm") < 250 || Units::convert(wavelength_, "nm") > 1450) {
            throw InvalidValueError(config_, "wavelength", "Currently supported wavelengths are 250 -- 1450 nm");
        }

        // Register lookup path for data files:
        if(config_.has("data_path")) {
            auto path = config_.getPath("data_path", true);
            if(!std::filesystem::is_directory(path)) {
                throw InvalidValueError(config_, "data_path", "path does not point to a directory");
            }
            LOG(TRACE) << "Registered absorption data path from configuration: " << path;
        } else {
            if(std::filesystem::is_directory(ALLPIX_LASER_DATA_DIRECTORY)) {
                config_.set<std::string>("data_path", ALLPIX_LASER_DATA_DIRECTORY);
                LOG(TRACE) << "Registered absorption data path from system: " << ALLPIX_LASER_DATA_DIRECTORY;
            } else {
                const char* data_dirs_env = std::getenv("XDG_DATA_DIRS");
                if(data_dirs_env == nullptr || strlen(data_dirs_env) == 0) {
                    data_dirs_env = "/usr/local/share/:/usr/share/:";
                }

                auto data_dirs = split<std::filesystem::path>(data_dirs_env, ":");
                for(auto& data_dir : data_dirs) {
                    data_dir /= std::filesystem::path(ALLPIX_PROJECT_NAME) / "data";
                    if(std::filesystem::is_directory(data_dir)) {
                        config_.set<std::string>("data_path", data_dir);
                        LOG(TRACE) << "Registered absorption data path from XDG_DATA_DIRS: " << data_dir;
                    } else {
                        throw ModuleError(
                            "Cannot find absorption data files, provide them in the configuration, via XDG_DATA_DIRS or "
                            "in system directory " +
                            std::string(ALLPIX_LASER_DATA_DIRECTORY));
                    }
                }
            }
        }
    }

    config_.setDefault<bool>("output_plots", false);
    output_plots_ = config.get<bool>("output_plots");
}

void DepositionLaserModule::initialize() {
    // Check if there are user-specified optical properties for materials
    if(!is_user_optics_) {
        // wavelength: {absorption_length, refractive_index}
        std::map<double, std::pair<double, double>> optics_lut;
        double wl = 0;
        double abs_length = 0;
        double refr_ind = 0;

        // Load data
        auto file_path = config_.getPath("data_path", true) / "silicon_photoabsorption.data";
        std::ifstream f(file_path);
        LOG(DEBUG) << "Loading optical properties for sensor material from LUT: " << std::endl << file_path.string();

        if(!f || !std::filesystem::is_regular_file(file_path)) {
            throw ModuleError("Could not open optical properties reference file at \"" + file_path.string() + "\"");
        }

        while(f >> wl >> abs_length >> refr_ind) {
            optics_lut[Units::get(wl, "nm")] = {abs_length, refr_ind};
        }

        // Find or interpolate absorption depth for given wavelength
        if(optics_lut.count(wavelength_) != 0) {
            absorption_length_ = optics_lut[wavelength_].first;
            refractive_index_ = optics_lut[wavelength_].second;
        } else {
            auto it = optics_lut.upper_bound(wavelength_);
            double wl1 = (*prev(it)).first;
            double wl2 = (*it).first;
            absorption_length_ =
                (optics_lut[wl1].first * (wl2 - wavelength_) + optics_lut[wl2].first * (wavelength_ - wl1)) / (wl2 - wl1);
            refractive_index_ =
                (optics_lut[wl1].second * (wl2 - wavelength_) + optics_lut[wl2].second * (wavelength_ - wl1)) / (wl2 - wl1);
        }
    }
    LOG(DEBUG) << "Wavelength = " << Units::display(wavelength_, "nm")
               << ", absorption length: " << Units::display(absorption_length_, {"um", "mm"})
               << ", refractive index: " << refractive_index_;

    // Check for unsupported detector materials, warn user if present

    std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
    for(auto& detector : detectors) {
        auto material = detector->getModel()->getSensorMaterial();
        if(material != SensorMaterial::SILICON && !is_user_optics_) {
            LOG(WARNING) << "Detector " << detector->getName() << " has unsupported material and will be ignored";
        }
    }
    // Check for incompatible passive objects, warn user if there are any
    auto passive_configs = geo_manager_->getPassiveElements();
    for(const auto& item : passive_configs) {
        auto shape = item.get<std::string>("type");
        if(shape != "box") {
            LOG(WARNING) << item.getName() << " passive object has unsupported type (" << shape << ") and will be ignored";
        }
    }

    // Create Histograms
    if(output_plots_) {
        LOG(DEBUG) << "Initializing histograms";
        Int_t nbins = 100;
        double nsigmas = 3;
        double focalplane_histsize = beam_waist_ * nsigmas;

        h_intensity_focalplane_ = CreateHistogram<TH2D>("intensity_focalplane",
                                                        "Beam profile in focal plane, a.u.;x [mm];y [mm]",
                                                        nbins,
                                                        -focalplane_histsize,
                                                        focalplane_histsize,
                                                        nbins,
                                                        -focalplane_histsize,
                                                        focalplane_histsize);

        double sourceplane_histsize = focalplane_histsize;
        if(beam_geometry_ == BeamGeometry::CONVERGING) {
            sourceplane_histsize += focal_distance_ * sin(beam_convergence_angle_);
        }

        h_intensity_sourceplane_ = CreateHistogram<TH2D>("intensity_sourceplane",
                                                         "Beam profile at source, a.u.;x [mm];y [mm]",
                                                         nbins,
                                                         -sourceplane_histsize,
                                                         sourceplane_histsize,
                                                         nbins,
                                                         -sourceplane_histsize,
                                                         sourceplane_histsize);

        h_angular_phi_ = CreateHistogram<TH1D>(
            "phi_distribution", "Phi_distribution w.r.t. beam direction;Phi [rad];Counts", nbins, -3.5, 3.5);
        h_angular_theta_ = CreateHistogram<TH1D>(
            "theta_distribution", "Theta distribution w.r.t. beam direction;Theta [deg];Counts", nbins, 0, 45);
        h_pulse_shape_ =
            CreateHistogram<TH1D>("pulse_shape", "Pulse shape;t [ns];Intensity [a.u.]", nbins, 0, 8 * pulse_duration_);

        for(const auto& detector : detectors) {
            std::string name = "dep_charge_" + detector->getName();
            std::string title = name + ";x [mm];y [mm];z [mm]";
            auto sensor = detector->getModel()->getSensorSize();

            h_deposited_charge_shapes_[detector] = CreateHistogram<TH3D>(name.c_str(),
                                                                         title.c_str(),
                                                                         100,
                                                                         -sensor.X() / 2,
                                                                         sensor.X() / 2,
                                                                         100,
                                                                         -sensor.Y() / 2,
                                                                         sensor.Y() / 2,
                                                                         100,
                                                                         -sensor.Z() / 2,
                                                                         sensor.Z() / 2);
        }
    }
}

void DepositionLaserModule::run(Event* event) {

    // Containers for output messages

    std::map<std::shared_ptr<Detector>, std::vector<MCParticle>> mc_particles;
    std::map<std::shared_ptr<Detector>, std::vector<DepositedCharge>> deposited_charges;

    // Lambda generator to yield pulse shape
    auto yield_starting_time = [&]() {
        int cut_sigmas = 4;
        double result = -1;
        while(result < 0) {
            result =
                allpix::normal_distribution<double>(cut_sigmas * pulse_duration_, pulse_duration_)(event->getRandomEngine());
        }
        return result;
    };

    // Containers for timestamps
    std::vector<double> starting_times(number_of_photons_);
    std::for_each(begin(starting_times), end(starting_times), [&](auto& item) {
        item = yield_starting_time();
        if(output_plots_) {
            h_pulse_shape_->Fill(item);
        }
    });

    std::sort(begin(starting_times), end(starting_times));

    // To correctly offset local time for each detector
    std::map<std::shared_ptr<Detector>, double> local_time_offsets;

    // Loop over photons in a single laser pulse
    // In time order
    for(size_t i_photon = 0; i_photon < number_of_photons_; ++i_photon) {

        LOG_PROGRESS(INFO, "photon_counter")
            << "Event " << event->number << ": photon " << i_photon + 1 << " of " << number_of_photons_;

        // Starting point and direction for this exact photon
        auto [starting_point, photon_direction] = generate_photon_geometry(event);

        // Get starting time in the pulse
        double starting_time = starting_times[i_photon];
        LOG(DEBUG) << "    Starting timestamp: " << Units::display(starting_time, "ns");

        // Generate penetration depth
        double penetration_depth =
            allpix::exponential_distribution<double>(1 / absorption_length_)(event->getRandomEngine());
        LOG(DEBUG) << "    Penetration depth: " << Units::display(penetration_depth, "um");

        // Perform tracking
        std::optional<PhotonHit> hit_opt = track(starting_point, photon_direction, penetration_depth);

        // If this photon did not hit any of the detectors, skip this iteration
        if(!hit_opt) {
            continue;
        }

        PhotonHit hit = hit_opt.value();

        // If this was the first hit in this detector in this event,
        // remember entry timestamp as local t=0 for this detector.
        // It is assumed that photon that is created earlier also hits earlier
        if(local_time_offsets.count(hit.detector) == 0) {
            local_time_offsets[hit.detector] = starting_time + hit.time_to_entry;
        }

        // Create and store corresponding MCParticle and DepositedCharge
        auto entry_local = hit.detector->getLocalPosition(hit.entry_global);
        auto hit_local = hit.detector->getLocalPosition(hit.hit_global);

        double time_entry_global = starting_time + hit.time_to_entry;
        double time_hit_global = starting_time + hit.time_to_hit;
        double time_entry_local = time_entry_global - local_time_offsets[hit.detector];
        double time_hit_local = time_hit_global - local_time_offsets[hit.detector];

        LOG(DEBUG) << "    Hit in " << hit.detector->getName();
        LOG(DEBUG) << "        global: " << Units::display(hit.hit_global, {"mm"}) << Units::display(time_hit_global, "ns");
        LOG(DEBUG) << "        local: " << Units::display(hit_local, {"mm"}) << Units::display(time_hit_local, "ns");

        if(output_plots_) {
            h_deposited_charge_shapes_[hit.detector]->Fill(hit_local.X(), hit_local.Y(), hit_local.Z());
        }

        // If that is a first hit in this detector, create map entries
        if(mc_particles.count(hit.detector) == 0) {
            mc_particles[hit.detector] = std::vector<MCParticle>();
        }
        if(deposited_charges.count(hit.detector) == 0) {
            deposited_charges[hit.detector] = std::vector<DepositedCharge>();
        }

        // Construct all necessary objects in-place
        // allpix::MCParticle
        mc_particles[hit.detector].emplace_back(entry_local,
                                                hit.entry_global,
                                                hit_local,
                                                hit.hit_global,
                                                22, // gamma
                                                time_entry_local,
                                                time_entry_global);
        // Count electrons and holes:
        mc_particles[hit.detector].back().setTotalDepositedCharge(2);

        // allpix::DepositedCharge for electron
        deposited_charges[hit.detector].emplace_back(hit_local,
                                                     hit.hit_global,
                                                     CarrierType::ELECTRON,
                                                     group_photons_, // value
                                                     time_hit_local,
                                                     time_hit_global);

        // allpix::DepositedCharge for hole
        deposited_charges[hit.detector].emplace_back(hit_local,
                                                     hit.hit_global,
                                                     CarrierType::HOLE,
                                                     group_photons_, // value
                                                     time_hit_local,
                                                     time_hit_global);

    } // loop over photons

    LOG(INFO) << "Registered hits in " << mc_particles.size() << " detectors";

    // After all the containers are filled, assign MCParticle links in DepositedCharges

    for(const auto& [detector, data] : mc_particles) {
        for(size_t i = 0; i < size(mc_particles[detector]); ++i) {
            deposited_charges[detector][2 * i].setMCParticle(&(data[i]));
            deposited_charges[detector][2 * i + 1].setMCParticle(&(data[i]));
        }
    }

    // Dispatch messages
    for(auto& [detector, data] : mc_particles) {
        LOG(INFO) << "    " << detector->getName() << ": " << data.size() << " hits";
        auto mcparticle_message = std::make_shared<MCParticleMessage>(std::move(data), detector);
        messenger_->dispatchMessage(this, std::move(mcparticle_message), event);
    }

    for(auto& [detector, data] : deposited_charges) {
        auto charge_message = std::make_shared<DepositedChargeMessage>(std::move(data), detector);
        messenger_->dispatchMessage(this, std::move(charge_message), event);
    }
}

void DepositionLaserModule::finalize() {
    if(output_plots_) {
        h_intensity_focalplane_->Write();
        h_intensity_sourceplane_->Write();
        h_angular_phi_->Write();
        h_angular_theta_->Write();
        h_pulse_shape_->Write();
        for(auto& [detector, histo] : h_deposited_charge_shapes_) {
            histo->Write();
        }
    }
}

std::pair<ROOT::Math::XYZPoint, ROOT::Math::XYZVector> DepositionLaserModule::generate_photon_geometry(Event* event) {
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

    ROOT::Math::XYZPoint starting_point{};
    ROOT::Math::XYZVector photon_direction{};

    // Generate starting point and direction in the beam
    if(beam_geometry_ == BeamGeometry::CONVERGING) {
        // Converging beam case

        // Generate correct position in the focal plane
        auto focal_position = source_position_ + beam_direction_ * focal_distance_ + beam_pos_smearing(beam_waist_);

        // Generate angles
        double phi = allpix::uniform_real_distribution<double>(0, 2 * TMath::Pi())(event->getRandomEngine());
        double cos_theta =
            allpix::uniform_real_distribution<double>(cos(beam_convergence_angle_), 1)(event->getRandomEngine());

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

    LOG(DEBUG) << "    Starting point: " << Units::display(starting_point, {"mm"}) << ", direction: " << photon_direction;

    return {starting_point, photon_direction};
}

std::optional<DepositionLaserModule::PhotonHit> DepositionLaserModule::track(const ROOT::Math::XYZPoint& position,
                                                                             const ROOT::Math::XYZVector& direction,
                                                                             double penetration_depth) const {

    // Lambda for angle calculation
    auto angle = [](const ROOT::Math::XYZVector& v1, const ROOT::Math::XYZVector& v2) {
        return acos(v1.Unit().Dot(v2.Unit()));
    };

    double c = TMath::C() * 100; // speed of light in mm/ns

    std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
    std::vector<std::pair<std::shared_ptr<Detector>, std::pair<double, double>>> intersection_segments;

    for(auto& detector : detectors) {
        if(detector->getModel()->getSensorMaterial() != SensorMaterial::SILICON && !is_user_optics_) {
            continue;
        }
        auto intersection = intersect_with_sensor(detector, position, direction);
        if(intersection) {
            intersection_segments.emplace_back(std::make_pair(detector, intersection.value()));
        }
    }

    if(intersection_segments.empty()) {
        LOG(DEBUG) << "No intersections with sensitive detectors";
        return std::nullopt;
    }

    auto it_first_detector = std::min_element(begin(intersection_segments),
                                              end(intersection_segments),
                                              [](const auto& p1, const auto& p2) { return p1.second < p2.second; });

    auto detector = it_first_detector->first;
    double t0 = it_first_detector->second.first;

    auto intersect_passive = intersect_with_passives(position, direction);
    if(intersect_passive) {
        if(intersect_passive.value().first < t0) {
            LOG(DEBUG) << "Absorbed by (" << intersect_passive.value().second << ") passive object";
            return std::nullopt;
        }
    }

    auto normal_vector = -1 * intersection_normal_vector(detector, position + direction * t0);

    double incidence_angle = angle(direction, normal_vector);
    double refraction_angle = asin(sin(incidence_angle) / refractive_index_);

    // Construct direction of the refracted ray
    auto binormal = direction.Cross(normal_vector);
    ROOT::Math::AxisAngle refraction_rotation(binormal, incidence_angle - refraction_angle);
    ROOT::Math::XYZVector new_direction = refraction_rotation(direction);

    LOG(DEBUG) << "    Intersection with " << detector->getName();
    LOG(DEBUG) << "        entry at " << Units::display(position + direction * t0, {"mm"});
    LOG(DEBUG) << "        normal at entry: " << normal_vector << ", binormal: " << binormal.Unit();
    LOG(DEBUG) << "        incidence angle: " << Units::display(incidence_angle, "deg")
               << ", refraction angle: " << Units::display(refraction_angle, "deg");
    LOG(DEBUG) << "        direction after refraction: " << new_direction;

    // Intersect the refracted ray with the detector
    auto intersection = intersect_with_sensor(detector, position + direction * t0, new_direction);
    auto [t0_refract, t1_refract] = intersection.value();
    double crossing_distance = t1_refract - t0_refract;

    LOG(DEBUG) << "        crossing_distance: " << Units::display(crossing_distance, {"um", "mm"});

    if(crossing_distance < penetration_depth) {
        LOG(DEBUG) << "    Photon is not absorbed";
        return std::nullopt;
    }

    // Construct a hit

    return PhotonHit{detector,
                     position + direction * t0,
                     position + direction * t0 + new_direction * penetration_depth,
                     t0 / c,
                     t0 / c + penetration_depth / c * refractive_index_};
}

std::optional<std::pair<double, double>>
DepositionLaserModule::intersect_with_sensor(const std::shared_ptr<const Detector>& detector,
                                             const ROOT::Math::XYZPoint& position_global,
                                             const ROOT::Math::XYZVector& direction_global) const {
    // Obtain total sensor size
    auto sensor = detector->getModel()->getSensorSize();

    // Transform original position and direction to a sensor-related coordinate system,

    // Construct transformation from the sensor system to the global one
    // * The rotation into the global coordinate system
    // * The shift from the origin to the detector position
    ROOT::Math::Rotation3D rotation_center(detector->getOrientation());
    ROOT::Math::Translation3D translation_center(static_cast<ROOT::Math::XYZVector>(detector->getPosition()));
    ROOT::Math::Transform3D transform_center(rotation_center, translation_center);

    // Apply inverse of that transformation
    auto position_local = transform_center.Inverse()(position_global);

    // Direction vector can directly be rotated
    auto direction_local = detector->getOrientation().Inverse()(direction_global);

    return LiangBarsky::intersectionDistances(direction_local, position_local, sensor);
}

std::optional<std::pair<double, std::string>>
DepositionLaserModule::intersect_with_passives(const ROOT::Math::XYZPoint& position_global,
                                               const ROOT::Math::XYZVector& direction_global) const {

    std::optional<std::pair<double, std::string>> result{};
    auto passive_configs = geo_manager_->getPassiveElements();

    for(const auto& item : passive_configs) {
        auto shape = item.get<std::string>("type");
        if(shape != "box") {
            continue;
        }

        auto [passive_position, passive_orientation] = geo_manager_->getPassiveElementOrientation(item.getName());
        auto passive_size = item.get<ROOT::Math::XYZVector>("size");

        ROOT::Math::Rotation3D rotation_center(passive_orientation);
        ROOT::Math::Translation3D translation_center(static_cast<ROOT::Math::XYZVector>(passive_position));
        ROOT::Math::Transform3D transform_center(rotation_center, translation_center);
        auto position_local = transform_center.Inverse()(position_global);
        auto direction_local = rotation_center.Inverse()(direction_global);

        auto intersect = LiangBarsky::intersectionDistances(direction_local, position_local, passive_size);

        if(!intersect) {
            continue;
        }

        double distance = intersect.value().first;

        if(!result) {
            result = {distance, item.getName()};
        }

        if(distance < result.value().first) {
            result = {distance, item.getName()};
        }
    }

    return result;
}

ROOT::Math::XYZVector DepositionLaserModule::intersection_normal_vector(const std::shared_ptr<const Detector>& detector,
                                                                        const ROOT::Math::XYZPoint& position_global) const {
    // Obtain total sensor size
    auto sensor = detector->getModel()->getSensorSize();

    // Transform original position and direction to a sensor-related coordinate system,

    // Construct transformation from the sensor system to the global one
    // * The rotation into the global coordinate system
    // * The shift from the origin to the detector position
    ROOT::Math::Rotation3D rotation_center(detector->getOrientation());
    ROOT::Math::Translation3D translation_center(static_cast<ROOT::Math::XYZVector>(detector->getPosition()));
    ROOT::Math::Transform3D transform_center(rotation_center, translation_center);

    // Apply inverse of that transformation
    auto position_local = transform_center.Inverse()(position_global);

    std::vector<double> distances_to_faces = {abs(position_local.X() - sensor.X() / 2),
                                              abs(position_local.X() + sensor.X() / 2),
                                              abs(position_local.Y() - sensor.Y() / 2),
                                              abs(position_local.Y() + sensor.Y() / 2),
                                              abs(position_local.Z() - sensor.Z() / 2),
                                              abs(position_local.Z() + sensor.Z() / 2)};

    std::vector<ROOT::Math::XYZVector> normals_to_faces = {
        {1, 0, 0},
        {-1, 0, 0},
        {0, 1, 0},
        {0, -1, 0},
        {0, 0, 1},
        {0, 0, -1},
    };

    auto iter_min = std::min_element(begin(distances_to_faces), end(distances_to_faces));
    size_t index_min = static_cast<size_t>(abs(iter_min - begin(distances_to_faces))); // avoid implicit conversion

    return rotation_center(normals_to_faces[index_min]);
}
