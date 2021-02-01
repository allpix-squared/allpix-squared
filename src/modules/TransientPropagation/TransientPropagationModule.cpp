/**
 * @file
 * @brief Implementation of charge propagation module with transient behavior simulation
 * @copyright Copyright (c) 2019-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TransientPropagationModule.hpp"

#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>

#include <Eigen/Core>

#include "core/utils/log.h"
#include "objects/PixelCharge.hpp"
#include "objects/PropagatedCharge.hpp"
#include "tools/runge_kutta.h"

using namespace allpix;
using namespace ROOT::Math;

TransientPropagationModule::TransientPropagationModule(Configuration& config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), messenger_(messenger) {
    using XYVectorInt = DisplacementVector2D<Cartesian2D<int>>;

    // Enable parallelization of this module if multithreading is enabled:
    enable_parallelization();

    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector:
    messenger_->bindSingle(this, &TransientPropagationModule::deposits_message_, MsgFlags::REQUIRED);

    // Seed the random generator with the module seed
    random_generator_.seed(getRandomSeed());

    // Set default value for config variables
    config_.setDefault<double>("timestep", Units::get(0.01, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<double>("temperature", 293.15);
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<XYVectorInt>("induction_matrix", XYVectorInt(3, 3));
    config_.setDefault<bool>("ignore_magnetic_field", false);
    config_.setDefault<double>("auger_coefficient", Units::get(2e-30, "cm*cm*cm*cm*cm*cm*/s"));

    // Copy some variables from configuration to avoid lookups:
    temperature_ = config_.get<double>("temperature");
    timestep_ = config_.get<double>("timestep");
    integration_time_ = config_.get<double>("integration_time");
    matrix_ = config_.get<XYVectorInt>("induction_matrix");
    auger_coeff_ = config_.get<double>("auger_coefficient");

    if(matrix_.x() % 2 == 0 || matrix_.y() % 2 == 0) {
        throw InvalidValueError(config_, "induction_matrix", "Odd number of pixels in x and y required.");
    }

    output_plots_ = config_.get<bool>("output_plots");

    // Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    electron_Vm_ = Units::get(1.53e9 * std::pow(temperature_, -0.87), "cm/s");
    electron_Ec_ = Units::get(1.01 * std::pow(temperature_, 1.55), "V/cm");
    electron_Beta_ = 2.57e-2 * std::pow(temperature_, 0.66);

    hole_Vm_ = Units::get(1.62e8 * std::pow(temperature_, -0.52), "cm/s");
    hole_Ec_ = Units::get(1.24 * std::pow(temperature_, 1.68), "V/cm");
    hole_Beta_ = 0.46 * std::pow(temperature_, 0.17);

    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature_;

    // Reference lifetime and doping concentrations, taken from:
    // https://doi.org/10.1016/0038-1101(82)90203-9
    // https://doi.org/10.1016/0038-1101(76)90022-8
    electron_lifetime_reference_ = Units::get(1e-5, "s");
    hole_lifetime_reference_ = Units::get(4.0e-4, "s");
    electron_doping_reference_ = Units::get(1e16, "/cm/cm/cm");
    hole_doping_reference_ = Units::get(7.1e15, "/cm/cm/cm");

    // Parameter for charge transport in magnetic field (approximated from graphs:
    // http://www.ioffe.ru/SVA/NSM/Semicond/Si/electric.html) FIXME
    electron_Hall_ = 1.15;
    hole_Hall_ = 0.9;
}

void TransientPropagationModule::init() {

    auto detector = getDetector();

    // Check for electric field
    if(!detector->hasElectricField()) {
        LOG(WARNING) << "This detector does not have an electric field.";
    }

    if(!detector_->hasWeightingPotential()) {
        throw ModuleError("This module requires a weighting potential.");
    }

    if(detector_->getElectricFieldType() == FieldType::LINEAR) {
        throw ModuleError("This module cannot be used with linear electric fields.");
    }

    // Check for doping profile
    has_doping_profile_ = detector->hasDopingProfile();

    // Check for magnetic field
    has_magnetic_field_ = detector->hasMagneticField();
    if(has_magnetic_field_) {
        if(config_.get<bool>("ignore_magnetic_field")) {
            has_magnetic_field_ = false;
            LOG(WARNING) << "A magnetic field is switched on, but is set to be ignored for this module.";
        } else {
            LOG(DEBUG) << "This detector sees a magnetic field.";
            magnetic_field_ = detector_->getMagneticField();
        }
    }

    if(output_plots_) {
        potential_difference_ =
            new TH1D("potential_difference",
                     "Weighting potential difference between two steps;#left|#Delta#phi_{w}#right| [a.u.];events",
                     500,
                     0,
                     1);
        induced_charge_histo_ = new TH1D("induced_charge_histo",
                                         "Induced charge per time, all pixels;Drift time [ns];charge [e]",
                                         static_cast<int>(integration_time_ / timestep_),
                                         0,
                                         static_cast<double>(Units::convert(integration_time_, "ns")));
        induced_charge_e_histo_ = new TH1D("induced_charge_e_histo",
                                           "Induced charge per time, electrons only, all pixels;Drift time [ns];charge [e]",
                                           static_cast<int>(integration_time_ / timestep_),
                                           0,
                                           static_cast<double>(Units::convert(integration_time_, "ns")));
        induced_charge_h_histo_ = new TH1D("induced_charge_h_histo",
                                           "Induced charge per time, holes only, all pixels;Drift time [ns];charge [e]",
                                           static_cast<int>(integration_time_ / timestep_),
                                           0,
                                           static_cast<double>(Units::convert(integration_time_, "ns")));
        step_length_histo_ = new TH1D("step_length_histo",
                                      "Step length;length [#mum];integration steps",
                                      100,
                                      0,
                                      static_cast<double>(Units::convert(0.25 * model_->getSensorSize().z(), "um")));

        drift_time_histo_ = new TH1D("drift_time_histo",
                                     "Drift time;Drift time [ns];charge carriers",
                                     static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                     0,
                                     static_cast<double>(Units::convert(integration_time_, "ns")));
    }
}

void TransientPropagationModule::run(unsigned int) {

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    // Loop over all deposits for propagation
    LOG(TRACE) << "Propagating charges in sensor";
    for(const auto& deposit : deposits_message_->getData()) {

        // Only process if within requested integration time:
        if(deposit.getLocalTime() > integration_time_) {
            LOG(DEBUG) << "Skipping charge carriers deposited beyond integration time: "
                       << Units::display(deposit.getGlobalTime(), "ns") << " global / "
                       << Units::display(deposit.getLocalTime(), {"ns", "ps"}) << " local";
            continue;
        }

        // Loop over all charges in the deposit
        unsigned int charges_remaining = deposit.getCharge();

        LOG(DEBUG) << "Set of charge carriers (" << deposit.getType() << ") on "
                   << Units::display(deposit.getLocalPosition(), {"mm", "um"});

        auto charge_per_step = config_.get<unsigned int>("charge_per_step");
        while(charges_remaining > 0) {
            // Define number of charges to be propagated and remove charges of this step from the total
            if(charge_per_step > charges_remaining) {
                charge_per_step = charges_remaining;
            }
            charges_remaining -= charge_per_step;
            std::map<Pixel::Index, Pulse> px_map;

            // Get position and propagate through sensor
            auto prop_pair =
                propagate(deposit.getLocalPosition(), deposit.getType(), charge_per_step, deposit.getLocalTime(), px_map);

            // Create a new propagated charge and add it to the list
            auto global_position = detector_->getGlobalPosition(prop_pair.first);
            PropagatedCharge propagated_charge(prop_pair.first,
                                               global_position,
                                               deposit.getType(),
                                               std::move(px_map),
                                               deposit.getLocalTime() + prop_pair.second,
                                               deposit.getGlobalTime() + prop_pair.second,
                                               &deposit);

            LOG(DEBUG) << " Propagated " << charge_per_step << " to " << Units::display(prop_pair.first, {"mm", "um"})
                       << " in " << Units::display(prop_pair.second, "ns") << " time, induced "
                       << Units::display(propagated_charge.getCharge(), {"e"});

            propagated_charges.push_back(std::move(propagated_charge));

            if(output_plots_) {
                drift_time_histo_->Fill(static_cast<double>(Units::convert(prop_pair.second, "ns")), charge_per_step);
            }
        }
    }

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, propagated_charge_message);
}

/**
 * Propagation is simulated using a parameterization for the electron mobility. This is used to calculate the electron
 * velocity at every point with help of the electric field map of the detector. A Runge-Kutta integration is applied in
 * multiple steps, adding a random diffusion to the propagating charge every step.
 */
std::pair<ROOT::Math::XYZPoint, double> TransientPropagationModule::propagate(const ROOT::Math::XYZPoint& pos,
                                                                              const CarrierType& type,
                                                                              const unsigned int charge,
                                                                              const double initial_time,
                                                                              std::map<Pixel::Index, Pulse>& pixel_map) {
    Eigen::Vector3d position(pos.x(), pos.y(), pos.z());

    // Define a lambda function to compute the carrier mobility
    // NOTE This function is typically the most frequently executed part of the framework and therefore the bottleneck
    auto carrier_mobility = [&](double efield_mag) {
        // Compute carrier mobility from constants and electric field magnitude
        if(type == CarrierType::ELECTRON) {
            return electron_Vm_ / electron_Ec_ /
                   std::pow(1. + std::pow(efield_mag / electron_Ec_, electron_Beta_), 1.0 / electron_Beta_);
        } else {
            return hole_Vm_ / hole_Ec_ / std::pow(1. + std::pow(efield_mag / hole_Ec_, hole_Beta_), 1.0 / hole_Beta_);
        }
    };

    // Define a function to compute the diffusion
    auto carrier_diffusion = [&](double efield_mag, double timestep) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * carrier_mobility(efield_mag);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        std::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        Eigen::Vector3d diffusion;
        for(int i = 0; i < 3; ++i) {
            diffusion[i] = gauss_distribution(random_generator_);
        }
        return diffusion;
    };

    // Survival probability of this charge carrier package, evaluated once
    std::uniform_real_distribution<double> survival(0, 1);
    auto survival_probability = survival(random_generator_);

    auto carrier_alive = [&](double doping_concentration, double time) -> bool {
        auto lifetime_srh = (type == CarrierType::ELECTRON ? electron_lifetime_reference_ : hole_lifetime_reference_) /
                            (1 + std::fabs(doping_concentration) /
                                     (type == CarrierType::ELECTRON ? electron_doping_reference_ : hole_doping_reference_));

        // auger lifetime model
        auto lifetime_auger = 1.0 / (auger_coeff_ * doping_concentration * doping_concentration);

        // combine the two
        auto lifetime = lifetime_srh;
        if(lifetime_auger > 0) {
            lifetime = (lifetime_srh * lifetime_auger) / (lifetime_srh + lifetime_auger);
        }

        return survival_probability > (1 - std::exp(-1 * time / lifetime));
    };

    // Define lambda functions to compute the charge carrier velocity with or without magnetic field
    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&)> carrier_velocity_noB =
        [&](double, const Eigen::Vector3d& cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        return static_cast<int>(type) * carrier_mobility(efield.norm()) * efield;
    };

    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&)> carrier_velocity_withB =
        [&](double, const Eigen::Vector3d& cur_pos) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        Eigen::Vector3d velocity;
        Eigen::Vector3d bfield(magnetic_field_.x(), magnetic_field_.y(), magnetic_field_.z());

        auto mob = carrier_mobility(efield.norm());
        auto exb = efield.cross(bfield);

        Eigen::Vector3d term1;
        double hallFactor = (type == CarrierType::ELECTRON ? electron_Hall_ : hole_Hall_);
        term1 = static_cast<int>(type) * mob * hallFactor * exb;

        Eigen::Vector3d term2 = mob * mob * hallFactor * hallFactor * efield.dot(bfield) * bfield;

        auto rnorm = 1 + mob * mob * hallFactor * hallFactor * bfield.dot(bfield);
        return static_cast<int>(type) * mob * (efield + term1 + term2) / rnorm;
    };

    // Create the runge kutta solver with an RKF5 tableau
    auto runge_kutta = make_runge_kutta(
        tableau::RK5, (has_magnetic_field_ ? carrier_velocity_withB : carrier_velocity_noB), timestep_, position);

    // Continue propagation until the deposit is outside the sensor
    Eigen::Vector3d last_position = position;
    bool within_sensor = true;
    bool is_alive = true;
    while(within_sensor && (initial_time + runge_kutta.getTime()) < integration_time_ && is_alive) {
        // Save previous position and time
        last_position = position;

        // Execute a Runge Kutta step
        auto step = runge_kutta.step();

        // Get the current result
        position = runge_kutta.getValue();

        // Get electric field at current position and fall back to empty field if it does not exist
        auto efield = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(position));

        // Apply diffusion step
        auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), timestep_);
        position += diffusion;
        runge_kutta.setValue(position);

        // Check if charge carrier is still alive:
        if(has_doping_profile_) {
            is_alive = carrier_alive(detector_->getDopingProfile(static_cast<ROOT::Math::XYZPoint>(position)),
                                     runge_kutta.getTime());
        }

        // Update step length histogram
        if(output_plots_) {
            step_length_histo_->Fill(static_cast<double>(Units::convert(step.value.norm(), "um")));
        }

        // Check for overshooting outside the sensor and correct for it:
        if(!detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(position))) {
            LOG(TRACE) << "Carrier outside sensor: " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
            // within_sensor = false;

            auto check_position = position;
            check_position.z() = last_position.z();
            // Correct for position in z by interpolation to increase precision:
            if(detector_->isWithinSensor(static_cast<ROOT::Math::XYZPoint>(check_position))) {
                // FIXME this currently depends in the direction of the drift
                if(position.z() > 0 && type == CarrierType::HOLE) {
                    LOG(DEBUG) << "Not stopping carrier " << type << " at "
                               << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um"});
                } else if(position.z() < 0 && type == CarrierType::ELECTRON) {
                    LOG(DEBUG) << "Not stopping carrier " << type << " at "
                               << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um"});
                } else {
                    within_sensor = false;
                }

                // Carrier left sensor on top or bottom surface, interpolate
                auto z_cur_border = std::fabs(position.z() - model_->getSensorSize().z() / 2.0);
                auto z_last_border = std::fabs(model_->getSensorSize().z() / 2.0 - last_position.z());
                auto z_total = z_cur_border + z_last_border;
                position = (z_last_border / z_total) * position + (z_cur_border / z_total) * last_position;
                LOG(TRACE) << "Moved carrier to: " << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"nm"});
            } else {
                within_sensor = false;
            }
        }

        // Find the nearest pixel
        auto xpixel = static_cast<int>(std::round(position.x() / model_->getPixelSize().x()));
        auto ypixel = static_cast<int>(std::round(position.y() / model_->getPixelSize().y()));
        LOG(TRACE) << "Moving carriers below pixel "
                   << Pixel::Index(static_cast<unsigned int>(xpixel), static_cast<unsigned int>(ypixel)) << " from "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(last_position), {"um", "mm"}) << " to "
                   << Units::display(static_cast<ROOT::Math::XYZPoint>(position), {"um", "mm"}) << ", "
                   << Units::display(initial_time + runge_kutta.getTime(), "ns");

        // Loop over NxN pixels:
        for(int x = xpixel - matrix_.x() / 2; x <= xpixel + matrix_.x() / 2; x++) {
            for(int y = ypixel - matrix_.y() / 2; y <= ypixel + matrix_.y() / 2; y++) {
                // Ignore if out of pixel grid
                if(!detector_->isWithinPixelGrid(x, y)) {
                    LOG(TRACE) << "Pixel (" << x << "," << y << ") skipped, outside the grid";
                    continue;
                }

                Pixel::Index pixel_index(static_cast<unsigned int>(x), static_cast<unsigned int>(y));
                auto ramo = detector_->getWeightingPotential(static_cast<ROOT::Math::XYZPoint>(position), pixel_index);
                auto last_ramo =
                    detector_->getWeightingPotential(static_cast<ROOT::Math::XYZPoint>(last_position), pixel_index);

                // Induced charge on electrode is q_int = q * (phi(x1) - phi(x0))
                auto induced = charge * (ramo - last_ramo) * static_cast<std::underlying_type<CarrierType>::type>(type);
                LOG(TRACE) << "Pixel " << pixel_index << " dPhi = " << (ramo - last_ramo) << ", induced " << type
                           << " q = " << Units::display(induced, "e");

                // Create pulse if it doesn't exist. Store induced charge in the returned pulse iterator
                auto pixel_map_iterator = pixel_map.emplace(pixel_index, Pulse(timestep_));
                pixel_map_iterator.first->second.addCharge(induced, initial_time + runge_kutta.getTime());

                if(output_plots_) {
                    potential_difference_->Fill(std::fabs(ramo - last_ramo));
                    induced_charge_histo_->Fill(initial_time + runge_kutta.getTime(), induced);
                    if(type == CarrierType::ELECTRON) {
                        induced_charge_e_histo_->Fill(initial_time + runge_kutta.getTime(), induced);
                    } else {
                        induced_charge_h_histo_->Fill(initial_time + runge_kutta.getTime(), induced);
                    }
                }
            }
        }
    }

    // Return the final position of the propagated charge
    return std::make_pair(static_cast<ROOT::Math::XYZPoint>(position), initial_time + runge_kutta.getTime());
}

void TransientPropagationModule::finalize() {
    if(output_plots_) {
        potential_difference_->Write();
        step_length_histo_->Write();
        drift_time_histo_->Write();
        induced_charge_histo_->Write();
        induced_charge_e_histo_->Write();
        induced_charge_h_histo_->Write();
    }
}
