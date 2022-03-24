/**
 * @file
 * @brief Implementation of ProjectionPropagation module
 * @copyright MIT License
 */

#include "ProjectionPropagationModule.hpp"

#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <utility>

#include "core/messenger/Messenger.hpp"
#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

using namespace allpix;

ProjectionPropagationModule::ProjectionPropagationModule(Configuration& config,
                                                         Messenger* messenger,
                                                         std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)),
      top_z_(detector_->getModel()->getSensorSize().z() / 2) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector
    messenger_->bindSingle<DepositedChargeMessage>(this, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<int>("charge_per_step", 10);
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<bool>("diffuse_deposit", false);
    config_.setDefault<std::string>("recombination_model", "none");

    integration_time_ = config_.get<double>("integration_time");
    output_plots_ = config_.get<bool>("output_plots");
    diffuse_deposit_ = config_.get<bool>("diffuse_deposit");
    charge_per_step_ = config_.get<unsigned int>("charge_per_step");

    // Set default for charge carrier propagation:
    config_.setDefault<bool>("propagate_holes", false);
    if(config_.get<bool>("propagate_holes")) {
        propagate_type_ = CarrierType::HOLE;
        LOG(INFO) << "Holes are chosen for propagation. Electrons are therefore not propagated.";
    } else {
        propagate_type_ = CarrierType::ELECTRON;
    }

    auto temperature = config_.get<double>("temperature");
    boltzmann_kT_ = Units::get(8.6173e-5, "eV/K") * temperature;

    // Mobility fixed to Jacoboni:
    mobility_ = std::make_unique<JacoboniCanali>(temperature);

    // We need direct access to the critical field values of the model since we have a discrete integration of the formula
    // for the total drift time. Taken from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2)
    electron_Ec_ = Units::get(1.01 * std::pow(temperature, 1.55), "V/cm");
    hole_Ec_ = Units::get(1.24 * std::pow(temperature, 1.68), "V/cm");

    config_.setDefault<bool>("ignore_magnetic_field", false);
}

void ProjectionPropagationModule::initialize() {
    if(detector_->getElectricFieldType() != FieldType::LINEAR) {
        throw ModuleError("This module should only be used with linear electric fields.");
    }

    if(detector_->hasDopingProfile() && detector_->getDopingProfileType() != FieldType::CONSTANT) {
        throw ModuleError("This module should only be used with constant doping concentration.");
    }

    // Prepare recombination model
    recombination_ = Recombination(config_, detector_->hasDopingProfile());

    if(detector_->hasMagneticField() && !config_.get<bool>("ignore_magnetic_field")) {
        throw ModuleError("This module should not be used with magnetic fields. Add the option 'ignore_magnetic_field' to "
                          "the configuration if you would like to continue.");
    } else if(detector_->hasMagneticField() && config_.get<bool>("ignore_magnetic_field")) {
        LOG(WARNING) << "A magnetic field is switched on, but is set to be ignored for this module.";
    }

    // Find correct top side
    if(detector_->getElectricField({0, 0, top_z_}).z() > detector_->getElectricField({0, 0, -top_z_}).z()) {
        top_z_ *= -1;
    }
    if(propagate_type_ == CarrierType::HOLE) {
        top_z_ *= -1;
    }

    if(top_z_ < 0) {
        LOG(WARNING)
            << "Selected carriers are not propagated to the implant side, combination of propagated carrier and electric "
               "field is wrong!";
    }

    if(output_plots_) {
        // Initialize output plots
        propagation_time_histo_ =
            CreateHistogram<TH1D>("propagation_time_histo",
                                  "Propagation time (drift + diffusion);Propagation time [ns];charge carriers",
                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")) * 2);
        drift_time_histo_ = CreateHistogram<TH1D>("drift_time_histo",
                                                  "Drift time (directed drift only);Drift time [ns];charge carriers",
                                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                  0,
                                                  static_cast<double>(Units::convert(integration_time_, "ns")) * 2);
        initial_position_histo_ =
            CreateHistogram<TH1D>("initial_position_histo",
                                  "Initial position of collected charge carriers;Position z [um];charge carriers",
                                  100,
                                  static_cast<double>(Units::convert(-top_z_, "um")),
                                  static_cast<double>(Units::convert(top_z_, "um")));

        recombine_histo_ =
            CreateHistogram<TH1D>("recombination_histo",
                                  "Fraction of recombined charge carriers;recombination [N / N_{total}] ;number of events",
                                  100,
                                  0,
                                  1);

        if(diffuse_deposit_) {
            diffusion_time_histo_ =
                CreateHistogram<TH1D>("diffusion_time_histo",
                                      "Diffusion time prior to drift;Diffusion time [ns];charge carriers",
                                      static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
        }
    }
}

void ProjectionPropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    unsigned int charge_lost = 0;
    unsigned int total_charge = 0;
    unsigned int total_projected_charge = 0;
    unsigned int recombined_charges_count = 0;

    // Loop over all deposits for propagation
    for(const auto& deposit : deposits_message->getData()) {

        auto type = deposit.getType();
        auto initial_position = deposit.getLocalPosition();

        // Selection of charge carrier:
        if(type != propagate_type_) {
            continue;
        }

        LOG(DEBUG) << "Set of " << deposit.getCharge() << " charge carriers (" << type << ") on "
                   << Units::display(initial_position, {"mm", "um"});

        unsigned int projected_charge = 0;

        unsigned int charges_remaining = deposit.getCharge();
        total_charge += charges_remaining;

        auto charge_per_step = charge_per_step_;
        while(charges_remaining > 0) {
            if(charge_per_step > charges_remaining) {
                charge_per_step = charges_remaining;
            }
            charges_remaining -= charge_per_step;

            auto position = initial_position;

            // Get the electric field at the position of the deposited charge and the top of the sensor:
            auto efield = detector_->getElectricField(position);
            double efield_mag = std::sqrt(efield.Mag2());
            auto efield_top = detector_->getElectricField(ROOT::Math::XYZPoint(0., 0., top_z_));
            double efield_mag_top = std::sqrt(efield_top.Mag2());
            double doping = detector_->getDopingConcentration(position);
            double diffusion_time = 0;

            // Only project if within the depleted region (i.e. efield not zero)
            if(efield_mag < std::numeric_limits<double>::epsilon()) {
                LOG(TRACE) << "Electric field is zero at " << Units::display(position, {"mm", "um"});
                if(!diffuse_deposit_) {
                    continue;
                }
                double diffusion_constant = boltzmann_kT_ * (*mobility_)(type, efield_mag, doping);
                double diffusion_std_dev = std::sqrt(2. * diffusion_constant * integration_time_);
                LOG(TRACE) << "Diffusion width of this charge carrier is " << Units::display(diffusion_std_dev, "um");

                allpix::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
                double diffusion_x = gauss_distribution(event->getRandomEngine());
                double diffusion_y = gauss_distribution(event->getRandomEngine());
                double diffusion_z = gauss_distribution(event->getRandomEngine());
                auto diffusion_vec = ROOT::Math::XYZVector(diffusion_x, diffusion_y, diffusion_z);

                auto local_position_diffusion = position + diffusion_vec;

                auto efield_diffusion = detector_->getElectricField(local_position_diffusion);
                double efield_mag_diffusion = std::sqrt(efield_diffusion.Mag2());

                if(efield_mag_diffusion < std::numeric_limits<double>::epsilon() && (detector_->isWithinSensor(position))) {
                    LOG(TRACE) << "Charge carrier remains within undepleted volume";
                    continue;
                }

                std::function<ROOT::Math::XYZPoint(const ROOT::Math::XYZPoint&, const ROOT::Math::XYZPoint&)> interval;
                interval = [this, &interval](const ROOT::Math::XYZPoint& start,
                                             const ROOT::Math::XYZPoint& stop) -> ROOT::Math::XYZPoint {
                    // Break nested intervals at a precision of 0.01 um
                    if(std::sqrt((stop - ROOT::Math::XYZVector(start)).Mag2()) < 0.00001) {
                        return stop;
                    }
                    auto efield_center = detector_->getElectricField((stop + ROOT::Math::XYZVector(start)) / 2.);
                    double efield_center_mag = std::sqrt(efield_center.Mag2());
                    if(efield_center_mag > std::numeric_limits<double>::epsilon()) {
                        return interval(start, (stop + ROOT::Math::XYZVector(start)) / 2.);
                    } else {
                        return interval((stop + ROOT::Math::XYZVector(start)) / 2., stop);
                    }
                };

                position = interval(position, local_position_diffusion);
                efield = detector_->getElectricField(position);
                efield_mag = std::sqrt(efield.Mag2());
                diffusion_time = integration_time_ * std::sqrt((position - initial_position).Mag2() /
                                                               (local_position_diffusion - initial_position).Mag2());

                if(!detector_->isWithinSensor(position)) {
                    LOG(TRACE) << "Charge carrier diffused outside the sensor volume";
                    continue;
                }
                LOG(TRACE) << "Charge diffused to position: " << Units::display(position, {"mm", "um"});
                LOG(TRACE) << " ... with an electric field of " << Units::display(efield_mag, "V/cm");
                LOG(TRACE) << " ... and a diffusion time prior to the drift of " << Units::display(diffusion_time, "ns");
            }

            LOG(TRACE) << "Electric field at carrier position / top of the sensor: "
                       << Units::display(efield_mag_top, "V/cm") << " , " << Units::display(efield_mag, "V/cm");

            auto slope_efield = (efield_mag_top - efield_mag) / (std::abs(top_z_ - position.z()));

            // Calculate the drift time
            auto calc_drift_time = [&]() {
                if(position.z() == top_z_) {
                    return 0.;
                }

                double Ec = (type == CarrierType::ELECTRON ? electron_Ec_ : hole_Ec_);

                return ((log(efield_mag_top) - log(efield_mag)) / slope_efield + std::abs(top_z_ - position.z()) / Ec) /
                       (*mobility_)(type, 0, doping);
            };
            LOG(TRACE) << "Electric field is " << Units::display(efield_mag, "V/cm");

            // Assume linear electric field over the depleted part of the sensor
            double diffusion_constant =
                boltzmann_kT_ * ((*mobility_)(type, efield_mag, doping) + (*mobility_)(type, efield_mag_top, doping)) / 2.;

            double drift_time = calc_drift_time();
            double propagation_time = deposit.getLocalTime() + drift_time + diffusion_time;
            LOG(TRACE) << "Drift time is " << Units::display(drift_time, "ns");

            if(output_plots_) {
                propagation_time_histo_->Fill(propagation_time, charge_per_step);
                drift_time_histo_->Fill(drift_time, charge_per_step);
                if(diffuse_deposit_) {
                    diffusion_time_histo_->Fill(diffusion_time, charge_per_step);
                }
            }

            double diffusion_std_dev = std::sqrt(2. * diffusion_constant * drift_time);
            LOG(TRACE) << "Diffusion width is " << Units::display(diffusion_std_dev, "um");

            // Check if charge carrier is still alive via its survival probability, evaluated once
            std::uniform_real_distribution<double> survival(0, 1);
            if(recombination_(
                   type, detector_->getDopingConcentration(position), survival(event->getRandomEngine()), drift_time)) {
                LOG(DEBUG) << "Recombined " << charge_per_step << " charge carriers (" << type << ") at "
                           << Units::display(position, {"mm", "um"});
                recombined_charges_count += charge_per_step;
                continue;
            }

            allpix::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
            double diffusion_x = gauss_distribution(event->getRandomEngine());
            double diffusion_y = gauss_distribution(event->getRandomEngine());

            // Find projected position
            auto local_position = ROOT::Math::XYZPoint(position.x() + diffusion_x, position.y() + diffusion_y, top_z_);

            auto global_time = deposit.getGlobalTime() + propagation_time;
            auto local_time = deposit.getLocalTime() + propagation_time;

            // Only add if within requested integration time:
            if(propagation_time > integration_time_) {
                LOG(DEBUG) << "Charge carriers propagation time not within integration time: "
                           << Units::display(global_time, "ns") << " global / " << Units::display(local_time, {"ns", "ps"})
                           << " local";
                continue;
            }

            // Only add if within sensor volume:
            if(!detector_->isWithinSensor(local_position)) {
                LOG(DEBUG) << "Charge carriers outside sensor volume at " << Units::display(local_position, {"mm", "um"});
                // FIXME: drop charges if it ends up outside the sensor, could be optimized to estimate position on border
                continue;
            }

            if(output_plots_) {
                initial_position_histo_->Fill(static_cast<double>(Units::convert(initial_position.z(), "um")),
                                              charge_per_step);
            }

            auto global_position = detector_->getGlobalPosition(local_position);

            // Produce charge carrier at this position
            propagated_charges.emplace_back(
                local_position, global_position, deposit.getType(), charge_per_step, local_time, global_time, &deposit);

            LOG(DEBUG) << "Propagated " << charge_per_step << " " << type << " to "
                       << Units::display(local_position, {"mm", "um"}) << " in " << Units::display(global_time, "ns")
                       << " global / " << Units::display(local_time, {"ns", "ps"}) << " local";

            projected_charge += charge_per_step;
        }
        total_projected_charge += projected_charge;
    }
    charge_lost = total_charge - total_projected_charge;

    if(total_charge > 0) {
        LOG(INFO) << "Total charge: " << total_charge << " (lost: " << charge_lost << ", "
                  << (charge_lost / total_charge * 100.) << "%)";
    }
    LOG(DEBUG) << "Total count of propagated charge carriers: " << propagated_charges.size();

    if(output_plots_) {
        recombine_histo_->Fill(static_cast<double>(recombined_charges_count) / total_charge);
    }

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, propagated_charge_message, event);
}

void ProjectionPropagationModule::finalize() {
    if(output_plots_) {
        // Write output plots
        drift_time_histo_->Write();
        propagation_time_histo_->Write();
        initial_position_histo_->Write();
        recombine_histo_->Write();
        if(diffuse_deposit_) {
            diffusion_time_histo_->Write();
        }
    }
}
