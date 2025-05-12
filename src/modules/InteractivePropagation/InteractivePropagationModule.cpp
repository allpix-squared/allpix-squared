/**
 * @file
 * @brief Implementation of InteractivePropagation module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "InteractivePropagationModule.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <Eigen/Core>

#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "objects/exceptions.h"
#include "tools/runge_kutta.h"

using namespace allpix;

InteractivePropagationModule::InteractivePropagationModule(Configuration& config,
    Messenger* messenger,
    std::shared_ptr<Detector> detector)
: Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {

    // Save detector model
    model_ = detector_->getModel();

    // Require deposits message for single detector:
    messenger_->bindSingle<DepositedChargeMessage>(this, MsgFlags::REQUIRED);

    // Set default value for config variables
    config_.setDefault<double>("timestep", Units::get(0.01, "ns"));
    config_.setDefault<double>("integration_time", Units::get(25, "ns"));
    config_.setDefault<unsigned int>("charge_per_step", 10);
    config_.setDefault<unsigned int>("max_charge_groups", 1000);

    // Models:
    config_.setDefault<std::string>("mobility_model", "jacoboni");
    config_.setDefault<std::string>("recombination_model", "none");
    config_.setDefault<std::string>("trapping_model", "none");
    config_.setDefault<std::string>("detrapping_model", "none");

    config_.setDefault<double>("temperature", 293.15);
    config_.setDefault<unsigned int>("distance", 1);
    config_.setDefault<bool>("ignore_magnetic_field", false);
    config_.setDefault<double>("surface_reflectivity", 0.0);

    // Set defaults for charge carrier multiplication
    config_.setDefault<double>("multiplication_threshold", 1e-2);
    config_.setDefault<unsigned int>("max_multiplication_level", 5);
    config_.setDefault<std::string>("multiplication_model", "none");

    config_.setDefault<bool>("output_linegraphs", false);
    config_.setDefault<bool>("output_linegraphs_collected", false);
    config_.setDefault<bool>("output_linegraphs_recombined", false);
    config_.setDefault<bool>("output_linegraphs_trapped", false);
    config_.setDefault<bool>("output_animations", false);
    config_.setDefault<bool>("output_plots",
    config_.get<bool>("output_linegraphs") || config_.get<bool>("output_animations"));
    config_.setDefault<bool>("output_animations_color_markers", false);
    config_.setDefault<double>("output_plots_step", config_.get<double>("timestep"));
    config_.setDefault<bool>("output_plots_use_pixel_units", false);
    config_.setDefault<bool>("output_plots_align_pixels", false);
    config_.setDefault<double>("output_plots_theta", 0.0f);
    config_.setDefault<double>("output_plots_phi", 0.0f);

    // Copy some variables from configuration to avoid lookups:
    temperature_ = config_.get<double>("temperature");
    timestep_ = config_.get<double>("timestep");
    integration_time_ = config_.get<double>("integration_time");
    distance_ = config_.get<unsigned int>("distance");
    charge_per_step_ = config_.get<unsigned int>("charge_per_step");
    max_charge_groups_ = config_.get<unsigned int>("max_charge_groups");
    boltzmann_kT_ = Units::get(8.6173333e-5, "eV/K") * temperature_;
    coulomb_K_ =  1.43996454e-12; //Units::get(1.43996454e-12, "MV mm/e");
    surface_reflectivity_ = config_.get<double>("surface_reflectivity");

    max_multiplication_level_ = config.get<unsigned int>("max_multiplication_level");

    relative_permativity_ = config.get<double>("relative_permativity"); // the permativity of materials isn't in allpix, so rely on user to pass this in

    output_plots_ = config_.get<bool>("output_plots");
    output_linegraphs_ = config_.get<bool>("output_linegraphs");
    output_linegraphs_collected_ = config_.get<bool>("output_linegraphs_collected");
    output_linegraphs_recombined_ = config_.get<bool>("output_linegraphs_recombined");
    output_linegraphs_trapped_ = config_.get<bool>("output_linegraphs_trapped");
    output_plots_step_ = config_.get<double>("output_plots_step");

    // Enable multithreading of this module if multithreading is enabled and no per-event output plots are requested:
    // FIXME: Review if this is really the case or we can still use multithreading
    if(!(config_.get<bool>("output_animations") || output_linegraphs_)) {
    allow_multithreading();
    } else {
    LOG(WARNING) << "Per-event line graphs or animations requested, disabling parallel event processing";
    }

    // Parameter for charge transport in magnetic field (approximated from graphs:
    // http://www.ioffe.ru/SVA/NSM/Semicond/Si/electric.html) FIXME
    electron_Hall_ = 1.15;
    hole_Hall_ = 0.9;
}

// Copied from TransientPropagation.cpp
void InteractivePropagationModule::initialize() {

    // Check for electric field
    if(!detector_->hasElectricField()) {
        LOG(WARNING) << "This detector does not have an electric field.";
    }

    if(!detector_->hasWeightingPotential()) {
        throw ModuleError("This module requires a weighting potential.");
    }

    if(detector_->getElectricFieldType() == FieldType::LINEAR) {
        LOG(ERROR) << "This module will likely produce unphysical results when applying linear electric fields.";
    }

    // Prepare mobility model
    mobility_ = Mobility(config_, model_->getSensorMaterial(), detector_->hasDopingProfile());

    // Prepare recombination model
    recombination_ = Recombination(config_, detector_->hasDopingProfile());

    // Prepare trapping model
    trapping_ = Trapping(config_);

    // Prepare detrapping model
    detrapping_ = Detrapping(config_);

    // Impact ionization model
    multiplication_ = ImpactIonization(config_);


    // TODO: Determine the distance from the model origin to electrodes in both z-directions
    auto model_size = model_->getSize();
    LOG(INFO) << "Model size: " << model_size.x() << ", " << model_size.y() << ", " << model_size.z();
    
    auto model_center = model_->getModelCenter();
    LOG(INFO) << "Model center: " << model_center.x() << ", " << model_center.y() << ", " << model_center.z();

    auto sensor_center = model_->getSensorCenter();
    LOG(INFO) << "Sensor center: " << sensor_center.x() << ", " << sensor_center.y() << ", " << sensor_center.z();

    auto matrix_center = model_->getMatrixCenter();
    LOG(INFO) << "matrix center: " << matrix_center.x() << ", " << matrix_center.y() << ", " << matrix_center.z();

    z_lim_neg_ = model_center.z() - model_size.z() / 2; //TODO: Correct algorithm to not assume it's in the center
    z_lim_pos_ = model_center.z() + model_size.z() / 2;

    // z_lim_neg_ = model_->intersect(ROOT::Math::XYZVector(0, 0, -1), ROOT::Math::XYZPoint(0,0,0));

    // Check multiplication and step size larger than a picosecond:
    if(!multiplication_.is<NoImpactIonization>() && timestep_ > 0.001) {
        LOG(WARNING) << "Charge multiplication enabled with maximum timestep larger than 1ps" << std::endl
                     << "This might lead to unphysical gain values.";
    }

    // Check for magnetic field
    has_magnetic_field_ = detector_->hasMagneticField();
    if(has_magnetic_field_) {
        if(config_.get<bool>("ignore_magnetic_field")) {
            has_magnetic_field_ = false;
            LOG(WARNING) << "A magnetic field is switched on, but is set to be ignored for this module.";
        } else {
            LOG(DEBUG) << "This detector sees a magnetic field.";
        }
    }

    if(output_plots_) {

        auto pitch_x = static_cast<double>(Units::convert(model_->getPixelSize().x(), "um"));
        auto pitch_y = static_cast<double>(Units::convert(model_->getPixelSize().y(), "um"));

        potential_difference_ = CreateHistogram<TH1D>(
            "potential_difference",
            "Weighting potential difference between two steps;#left|#Delta#phi_{w}#right| [a.u.];events",
            500,
            0,
            1);
        induced_charge_histo_ = CreateHistogram<TH1D>("induced_charge_histo",
                                                      "Induced charge per time, all pixels;Drift time [ns];charge [e]",
                                                      static_cast<int>(integration_time_ / timestep_),
                                                      0,
                                                      static_cast<double>(Units::convert(integration_time_, "ns")));
        induced_charge_e_histo_ =
            CreateHistogram<TH1D>("induced_charge_e_histo",
                                  "Induced charge per time, electrons only, all pixels;Drift time [ns];charge [e]",
                                  static_cast<int>(integration_time_ / timestep_),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        induced_charge_h_histo_ =
            CreateHistogram<TH1D>("induced_charge_h_histo",
                                  "Induced charge per time, holes only, all pixels;Drift time [ns];charge [e]",
                                  static_cast<int>(integration_time_ / timestep_),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        if(!multiplication_.is<NoImpactIonization>()) {
            induced_charge_primary_histo_ =
                CreateHistogram<TH1D>("induced_charge_primary_histo",
                                      "Induced charge per time, primaries only, all pixels;Drift time [ns];charge [e]",
                                      static_cast<int>(integration_time_ / timestep_),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_primary_e_histo_ = CreateHistogram<TH1D>(
                "induced_charge_primary_e_histo",
                "Induced charge per time, primary electrons only, all pixels;Drift time [ns];charge [e]",
                static_cast<int>(integration_time_ / timestep_),
                0,
                static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_primary_h_histo_ =
                CreateHistogram<TH1D>("induced_charge_primary_h_histo",
                                      "Induced charge per time, primary holes only, all pixels;Drift time [ns];charge [e]",
                                      static_cast<int>(integration_time_ / timestep_),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_secondary_histo_ =
                CreateHistogram<TH1D>("induced_charge_secondary_histo",
                                      "Induced charge per time, secondaries only, all pixels;Drift time [ns];charge [e]",
                                      static_cast<int>(integration_time_ / timestep_),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_secondary_e_histo_ = CreateHistogram<TH1D>(
                "induced_charge_secondary_e_histo",
                "Induced charge per time, secondary electrons only, all pixels;Drift time [ns];charge [e]",
                static_cast<int>(integration_time_ / timestep_),
                0,
                static_cast<double>(Units::convert(integration_time_, "ns")));
            induced_charge_secondary_h_histo_ =
                CreateHistogram<TH1D>("induced_charge_secondary_h_histo",
                                      "Induced charge per time, secondary holes only, all pixels;Drift time [ns];charge [e]",
                                      static_cast<int>(integration_time_ / timestep_),
                                      0,
                                      static_cast<double>(Units::convert(integration_time_, "ns")));
        }
        induced_charge_vs_depth_histo_ =
            CreateHistogram<TH2D>("induced_charge_vs_depth_histo",
                                  "Induced charge per time vs depth, all pixels;Drift time [ns];depth [mm];charge [e]",
                                  static_cast<int>(integration_time_ / timestep_),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")),
                                  100,
                                  -model_->getSensorSize().z() / 2.,
                                  model_->getSensorSize().z() / 2.);
        induced_charge_e_vs_depth_histo_ = CreateHistogram<TH2D>(
            "induced_charge_e_vs_depth_histo",
            "Induced charge per time vs depth, electrons only, all pixels;Drift time [ns];depth [mm];charge [e]",
            static_cast<int>(integration_time_ / timestep_),
            0,
            static_cast<double>(Units::convert(integration_time_, "ns")),
            100,
            -model_->getSensorSize().z() / 2.,
            model_->getSensorSize().z() / 2.);
        induced_charge_h_vs_depth_histo_ = CreateHistogram<TH2D>(
            "induced_charge_h_vs_depth_histo",
            "Induced charge per time vs depth, holes only, all pixels;Drift time [ns];depth [mm];charge [e]",
            static_cast<int>(integration_time_ / timestep_),
            0,
            static_cast<double>(Units::convert(integration_time_, "ns")),
            100,
            -model_->getSensorSize().z() / 2.,
            model_->getSensorSize().z() / 2.);
        induced_charge_map_ = CreateHistogram<TH2D>(
            "induced_charge_map",
            "Induced charge as a function of in-pixel carrier position;x%pitch [#mum];y%pitch [#mum];charge [e]",
            static_cast<int>(pitch_x),
            -pitch_x / 2,
            pitch_x / 2,
            static_cast<int>(pitch_y),
            -pitch_y / 2,
            pitch_y / 2);
        induced_charge_e_map_ = CreateHistogram<TH2D>("induced_charge_e_map",
                                                      "Induced charge as a function of in-pixel carrier position, electrons "
                                                      "only;x%pitch [#mum];y%pitch [#mum];charge [e]",
                                                      static_cast<int>(pitch_x),
                                                      -pitch_x / 2,
                                                      pitch_x / 2,
                                                      static_cast<int>(pitch_y),
                                                      -pitch_y / 2,
                                                      pitch_y / 2);
        induced_charge_h_map_ = CreateHistogram<TH2D>(
            "induced_charge_h_map",
            "Induced charge as a function of in-pixel carrier position, holes only;x%pitch [#mum];y%pitch [#mum];charge [e]",
            static_cast<int>(pitch_x),
            -pitch_x / 2,
            pitch_x / 2,
            static_cast<int>(pitch_y),
            -pitch_y / 2,
            pitch_y / 2);

        step_length_histo_ =
            CreateHistogram<TH1D>("step_length_histo",
                                  "Step length;length [#mum];integration steps",
                                  100,
                                  0,
                                  static_cast<double>(Units::convert(0.25 * model_->getSensorSize().z(), "um")));
        group_size_histo_ = CreateHistogram<TH1D>("group_size_histo",
                                                  "Group size;size [charges];Number of groups",
                                                  static_cast<int>(100 * charge_per_step_),
                                                  0,
                                                  static_cast<int>(100 * charge_per_step_));

        drift_time_histo_ = CreateHistogram<TH1D>("drift_time_histo",
                                                  "Drift time;Drift time [ns];charge carriers",
                                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                  0,
                                                  static_cast<double>(Units::convert(integration_time_, "ns")));

        recombine_histo_ =
            CreateHistogram<TH1D>("recombination_histo",
                                  "Fraction of recombined charge carriers;recombination [N / N_{total}] ;number of events",
                                  100,
                                  0,
                                  1);
        recombination_time_histo_ =
            CreateHistogram<TH1D>("recombination_time_histo",
                                  "Time until recombination of charge carriers;time [ns];charge carriers",
                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));
        trapped_histo_ = CreateHistogram<TH1D>(
            "trapping_histo", "Fraction of trapped charge carriers;trapping [N / N_{total}] ;number of events", 100, 0, 1);
        trapping_time_histo_ = CreateHistogram<TH1D>("trapping_time_histo",
                                                     "Local time of trapping of charge carriers;time [ns];charge carriers",
                                                     static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                                     0,
                                                     static_cast<double>(Units::convert(integration_time_, "ns")));
        detrapping_time_histo_ =
            CreateHistogram<TH1D>("detrapping_time_histo",
                                  "Time from trapping until detrapping of charge carriers;time [ns];charge carriers",
                                  static_cast<int>(Units::convert(integration_time_, "ns") * 5),
                                  0,
                                  static_cast<double>(Units::convert(integration_time_, "ns")));

        if(!multiplication_.is<NoImpactIonization>()) {
            gain_primary_histo_ = CreateHistogram<TH1D>(
                "gain_primary_histo",
                "Gain per primarily induced charge carrier group after propagation;gain;number of groups transported",
                24,
                1,
                25);
            gain_all_histo_ =
                CreateHistogram<TH1D>("gain_all_histo",
                                      "Gain per charge carrier group after propagation;gain;number of groups transported",
                                      24,
                                      1,
                                      25);
            gain_e_histo_ =
                CreateHistogram<TH1D>("gain_e_histo",
                                      "Gain per primary electron group after propagation;gain;number of groups transported",
                                      24,
                                      1,
                                      25);
            gain_h_histo_ =
                CreateHistogram<TH1D>("gain_h_histo",
                                      "Gain per primary hole group after propagation;gain;number of groups transported",
                                      24,
                                      1,
                                      25);
            multiplication_level_histo_ = CreateHistogram<TH1D>(
                "multiplication_level_histo",
                "Multiplication level of propagated charge carriers;multiplication level;charge carriers",
                static_cast<int>(max_multiplication_level_),
                0,
                static_cast<int>(max_multiplication_level_));
            multiplication_depth_histo_ =
                CreateHistogram<TH1D>("multiplication_depth_histo",
                                      "Generation depth of charge carriers via impact ionization;depth [mm];charge carriers",
                                      200,
                                      -model_->getSensorSize().z() / 2.,
                                      model_->getSensorSize().z() / 2.);
            gain_e_vs_x_ =
                CreateHistogram<TProfile>("gain_e_vs_x",
                                          "Gain per electron group after propagation vs x; x [mm]; gain per group",
                                          100,
                                          -model_->getSensorSize().x() / 2.,
                                          model_->getSensorSize().x() / 2.);
            gain_e_vs_y_ =
                CreateHistogram<TProfile>("gain_e_vs_y",
                                          "Gain per electron group after propagation vs y; x [mm]; gain per group",
                                          100,
                                          -model_->getSensorSize().y() / 2.,
                                          model_->getSensorSize().y() / 2.);
            gain_e_vs_z_ =
                CreateHistogram<TProfile>("gain_e_vs_z",
                                          "Gain per electron group after propagation vs z; x [mm]; gain per group",
                                          100,
                                          -model_->getSensorSize().z() / 2.,
                                          model_->getSensorSize().z() / 2.);
            gain_h_vs_x_ = CreateHistogram<TProfile>("gain_h_vs_x",
                                                     "Gain per hole group after propagation vs x; x [mm]; gain per group",
                                                     100,
                                                     -model_->getSensorSize().x() / 2.,
                                                     model_->getSensorSize().x() / 2.);
            gain_h_vs_y_ = CreateHistogram<TProfile>("gain_h_vs_y",
                                                     "Gain per hole group after propagation vs y; x [mm]; gain per group",
                                                     100,
                                                     -model_->getSensorSize().y() / 2.,
                                                     model_->getSensorSize().y() / 2.);
            gain_h_vs_z_ = CreateHistogram<TProfile>("gain_h_vs_z",
                                                     "Gain per hole group after propagation vs z; x [mm]; gain per group",
                                                     100,
                                                     -model_->getSensorSize().z() / 2.,
                                                     model_->getSensorSize().z() / 2.);
        }
    }
}

// Copied from TransientPropagation.cpp with major modifications
void InteractivePropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;
    // unsigned int propagated_charges_count = 0;
    // unsigned int recombined_charges_count = 0;
    // unsigned int trapped_charges_count = 0;

    // List of points to plot to plot for output plots
    LineGraph::OutputPlotPoints output_plot_points;

    // Create vector of propagating charges to store each charge groups position, location, time, type, etc.
    std::vector<PropagatedCharge> propagating_charges;

    // Loop over all deposits for propagation
    for(const auto& deposit : deposits_message->getData()) {

        // Only process if within requested integration time: 
        if(deposit.getLocalTime() > integration_time_) {
            LOG(DEBUG) << "Skipping charge carriers deposited beyond integration time: "
                       << Units::display(deposit.getGlobalTime(), "ns") << " global / "
                       << Units::display(deposit.getLocalTime(), {"ns", "ps"}) << " local"
                       << "> Integration Time of " << integration_time_;
            continue;
        }

        total_deposits_++;

        // Loop over all charges in the deposit
        unsigned int charges_remaining = deposit.getCharge();

        LOG(DEBUG) << "Set of charge carriers (" << deposit.getType() << ") on "
                   << Units::display(deposit.getLocalPosition(), {"mm", "um"});

        auto charge_per_step = charge_per_step_;
        if(max_charge_groups_ > 0 && deposit.getCharge() / charge_per_step > max_charge_groups_) {
            charge_per_step = static_cast<unsigned int>(ceil(static_cast<double>(deposit.getCharge()) / max_charge_groups_));
            deposits_exceeding_max_groups_++;
            LOG(INFO) << "Deposited charge: " << deposit.getCharge()
                      << ", which exceeds the maximum number of charge groups allowed. Increasing charge_per_step to "
                      << charge_per_step << " for this deposit.";
        }
    
        while(charges_remaining > 0) {
            // Define number of charges to be propagated and remove charges of this step from the total
            if(charge_per_step > charges_remaining) {
                charge_per_step = charges_remaining;
            }
            charges_remaining -= charge_per_step;

            // Add charge to propagating charge vector to be time-stepped later
            PropagatedCharge propagating_charge(deposit.getLocalPosition(),
                                            deposit.getGlobalPosition(),
                                            deposit.getType(),
                                            charge_per_step,
                                            deposit.getLocalTime(), // The local deposition time
                                            deposit.getGlobalTime(), // The global deposition time
                                            CarrierState::MOTION,
                                            &deposit);

            propagating_charges.push_back(std::move(propagating_charge));

            // Add point of deposition to the output plots if requested
            if(output_linegraphs_) {
                output_plot_points.emplace_back(std::make_tuple(deposit.getGlobalTime(), charge_per_step, deposit.getType(), CarrierState::MOTION),
                                                std::vector<ROOT::Math::XYZPoint>());
            }
        }
    }

    LOG(INFO) << "Number of charge groups: " << propagating_charges.size(); 

    //Now we have a list of charges to propagate
    auto [recombined_charges_count, trapped_charges_count, propagated_charges_count] = propagate_together(event, propagating_charges, propagated_charges, output_plot_points);

    // Output plots if required
    if(output_linegraphs_) {
        LineGraph::Create(event->number, this, config_, output_plot_points, CarrierState::UNKNOWN);
        if(output_linegraphs_collected_) {
            LineGraph::Create(event->number, this, config_, output_plot_points, CarrierState::HALTED);
        }
        if(output_linegraphs_recombined_) {
            LineGraph::Create(event->number, this, config_, output_plot_points, CarrierState::RECOMBINED);
        }
        if(output_linegraphs_trapped_) {
            LineGraph::Create(event->number, this, config_, output_plot_points, CarrierState::TRAPPED);
        }
        if(config_.get<bool>("output_animations")) {
            LineGraph::Animate(event->number, this, config_, output_plot_points);
        }
    }

    LOG(INFO) << "Propagated " << propagated_charges_count << " charges" << std::endl
              << "Recombined " << recombined_charges_count << " charges during transport" << std::endl
              << "Trapped " << trapped_charges_count << " charges during transport";

    if(output_plots_) {
        auto total = (propagated_charges_count + recombined_charges_count + trapped_charges_count);
        recombine_histo_->Fill(static_cast<double>(recombined_charges_count) / (total == 0 ? 1 : total));
        trapped_histo_->Fill(static_cast<double>(trapped_charges_count) / (total == 0 ? 1 : total));
    }

    // Create a new message with propagated charges
    auto propagated_charge_message = std::make_shared<PropagatedChargeMessage>(std::move(propagated_charges), detector_);

    // Dispatch the message with propagated charges
    messenger_->dispatchMessage(this, std::move(propagated_charge_message), event);
}

std::tuple<unsigned int, unsigned int, unsigned int>
InteractivePropagationModule::propagate_together(Event* event,
                                                 std::vector<PropagatedCharge>& propagating_charges,
                                                 std::vector<PropagatedCharge>& propagated_charges,
                                                 LineGraph::OutputPlotPoints& output_plot_points) const {

    // Set up functions and variables that should stay constant throughout time:
    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;
    
    auto output_plot_index = output_plot_points.size() - 1;

    // Define a function to compute the diffusion
    auto carrier_diffusion = [&](double efield_mag, double doping, double timestep, allpix::CarrierType type) -> Eigen::Vector3d {
        double diffusion_constant = boltzmann_kT_ * mobility_(type, efield_mag, doping);
        double diffusion_std_dev = std::sqrt(2. * diffusion_constant * timestep);

        // Compute the independent diffusion in three
        allpix::normal_distribution<double> gauss_distribution(0, diffusion_std_dev);
        auto x = gauss_distribution(event->getRandomEngine());
        auto y = gauss_distribution(event->getRandomEngine());
        auto z = gauss_distribution(event->getRandomEngine());
        return {x, y, z};
    };

    // Survival probability of this charge carrier package, evaluated at every step
    allpix::uniform_real_distribution<double> uniform_distribution(0, 1);

    // Create vectors that store charge information in a place that can be modified (need to be here since they are used in dynamic field, but they are set to initial states below)
    std::vector<ROOT::Math::XYZPoint> charge_locations;
    // std::vector<int> charge_amounts;
    std::vector<double> charge_times;
    std::vector<allpix::CarrierState> charge_states;
    double_t time = 0;
    int numSamePos = 0;

    //Define a function to compute the e-field given a desired local point and the list of charges
        //TODO: Implement a filter to only calculate for charge with the same time (or within a certain time range) as well as Trapped charges that have larger time
    auto dynamic_efield = [&](ROOT::Math::XYZPoint point) -> Eigen::Vector3d {

        // for now just return nothing for the dynamic field
        // return Eigen::Vector3d(0,0,0);

        double dist_threshold_squared = 0.25; //mm

        ROOT::Math::XYZVector field = ROOT::Math::XYZVector(0,0,0);

        bool foundSamePos = false; // Check to pass over current
        // int numSamePos = 0;
        for (int i = 0; i < charge_locations.size(); i++){

            // Only get fields from charges that have deposition time less than the current time (skip the ones that haven't been deposited yet)
            // This means that trapped charges at future times are okay, but not charges that haven't been deposited yet
            // Charges that have reached the sensor (HALTED) are assumed to be swept away
            if (propagating_charges[i].getLocalTime() > time || charge_states[i] == allpix::CarrierState::HALTED){
                continue;
            }

            ROOT::Math::XYZPoint local_position = charge_locations[i];
            if (local_position == point){
                numSamePos += 1;
                if (!foundSamePos){
                    numSamePos -= 1; // ignore the actual same charge (one per loop)
                    continue;
                }
                // local_position = // TODO: move randomly the overlapping charges
            }
            // if (local_position == point && !foundSamePos){ // This just skips unnecessary calculations for dist_vector = 0. Replace with above code if implementing random offsets.
            //     foundSamePos = true;
            //     continue;
            // }

            // Get the correct signed charge
            int q = propagating_charges[i].getCharge();
            if (propagating_charges[i].getType() == allpix::CarrierType::ELECTRON){
                q *= -1;
            }
            
            auto dist_vector = point - local_position; // A vector between the desired points (mm?)
            auto dist_mag2 = std::min(dist_vector.Mag2(), dist_threshold_squared); // limit the distance to prevent numerical explosion
            auto dist_mag = ROOT::Math::sqrt(dist_mag2);

            // if (dist_vector.x() < dist_threshold && dist_vector.y() < dist_threshold && dist_vector.z() < dist_threshold){
            //     allpix::uniform_real_distribution<double> gauss_distribution(dist_threshold, dist_threshold*2);
            //     auto x = gauss_distribution(event->getRandomEngine());
            //     auto y = gauss_distribution(event->getRandomEngine());
            //     auto z = gauss_distribution(event->getRandomEngine());
            //     local_position = local_position + dist_vector/(dist_mag+dist_threshold/10)*dist_threshold + 
            // }
            
            field = field + dist_vector * (coulomb_K_ / relative_permativity_ * q / dist_mag2 / dist_mag); // Add the charges field to the net field at the point

            // Now do the same for the mirror charges based on z_lim_neg and z_lim_pos
            // Note: this assumes a parallel plate sensor (symmetry about z) in order to simplify the poisson equation to the mirror charge solution (potential is constant on each plate)
            ROOT::Math::XYZPoint mirror_position_neg = ROOT::Math::XYZPoint(local_position.x(), local_position.y(), 2*z_lim_neg_ - local_position.z());
            ROOT::Math::XYZPoint mirror_position_pos = ROOT::Math::XYZPoint(local_position.x(), local_position.y(), 2*z_lim_pos_ - local_position.z());

            // Apply field for negative mirror charge
            dist_vector = point - mirror_position_neg; // reuse the same variables to save some time
            dist_mag2 = std::min(dist_vector.Mag2(), dist_threshold_squared);
            dist_mag = ROOT::Math::sqrt(dist_mag2);

            field = field + dist_vector * (coulomb_K_ / relative_permativity_ * -1*q / dist_mag2 / dist_mag); // Mirror charges have opposite charge

            // Apply field for positive mirror charge
            dist_vector = point - mirror_position_pos;
            dist_mag2 = std::min(dist_vector.Mag2(), dist_threshold_squared);
            dist_mag = ROOT::Math::sqrt(dist_mag2);

            field = field + dist_vector * (coulomb_K_ / relative_permativity_ * -1*q / dist_mag2 / dist_mag);
            
        }

        Eigen::Vector3d output = Eigen::Vector3d(field.x(),field.y(),field.z());

        if (time < 4 && std::fmod(time, 3.5) < timestep_/2){
            LOG(DEBUG) << time << "ns: field = (" << output.x() << ", " << output.y() << ", " << output.z() << ")";
        }

        // if (numSamePos > 0){
        //     LOG(DEBUG) << numSamePos << " charges skipped due to overlapping positions at time " << time << "ns";
        // }

        return output;
    };

    // Define lambda functions to compute the charge carrier velocity with or without magnetic field
    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&, allpix::CarrierType type)> carrier_velocity_noB =
        [&](double, const Eigen::Vector3d& cur_pos, allpix::CarrierType type) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());
        efield = efield + dynamic_efield(static_cast<ROOT::Math::XYZPoint>(cur_pos)); // Modified to include dynamic field from concurrent charges

        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(cur_pos));

        return static_cast<int>(type) * mobility_(type, efield.norm(), doping) * efield;
    };

    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&, allpix::CarrierType type)> carrier_velocity_withB =
        [&](double, const Eigen::Vector3d& cur_pos, allpix::CarrierType type) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z()); 
        efield = efield + dynamic_efield(static_cast<ROOT::Math::XYZPoint>(cur_pos)); // Modified to include dynamic field from concurrent charges

        Eigen::Vector3d velocity;
        auto magnetic_field = detector_->getMagneticField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d bfield(magnetic_field.x(), magnetic_field.y(), magnetic_field.z());

        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(cur_pos));

        auto mob = mobility_(type, efield.norm(), doping);
        auto exb = efield.cross(bfield);

        Eigen::Vector3d term1;
        double hallFactor = (type == CarrierType::ELECTRON ? electron_Hall_ : hole_Hall_);
        term1 = static_cast<int>(type) * mob * hallFactor * exb;

        Eigen::Vector3d term2 = mob * mob * hallFactor * hallFactor * efield.dot(bfield) * bfield;

        auto rnorm = 1 + mob * mob * hallFactor * hallFactor * bfield.dot(bfield);
        return static_cast<int>(type) * mob * (efield + term1 + term2) / rnorm;
    };

    //Functions that convert between ROOT::Math:XYZPoint and Eigen::Vector3d (Eigen::Matrix<double, 3, 1>)
    auto convertPointToVector = [] (ROOT::Math::XYZPoint point) -> Eigen::Vector3d {
        return Eigen::Vector3d(point.x(), point.y(), point.z());
    };

    auto convertVectorToPoint = [] (Eigen::Vector3d vector) -> ROOT::Math::XYZPoint {
        return ROOT::Math::XYZPoint(vector.x(), vector.y(), vector.z());
    };

    // Create the pixel map which is used to collect the pulse objects
    std::vector< std::map<Pixel::Index, Pulse> > pixel_map_vector;

    // Create list of RK4 objects that correspond to each particle
    std::vector<allpix::RungeKutta<double, 4, 3>> runge_kutta_vector;

    // Initialization loop through all charges
    for(auto charge : propagating_charges){
        
        std::function<Eigen::Matrix<double, 3, 1>(double, Eigen::Matrix<double, 3, 1>)> step_function;
        if (has_magnetic_field_){
            step_function = [=](double t, Eigen::Vector3d pos) -> Eigen::Vector3d {return carrier_velocity_withB(t, pos, charge.getType());};
        }else{
            step_function = [=](double t, Eigen::Vector3d pos) -> Eigen::Vector3d {return carrier_velocity_noB(t, pos, charge.getType());};
        }

        // No error estimation required since we're not adapting step size
        auto rk = make_runge_kutta(
            tableau::RK4, 
            step_function, // This step function can only have two arguments (so I determine charge type before here)
            timestep_, 
            convertPointToVector(charge.getLocalPosition()));

        rk.advanceTime(charge.getLocalTime()); // Set the start time of each to the local time of the charges deposition
                                                
        runge_kutta_vector.push_back(rk); // Pass by value
                                    //  Eigen::Matrix<double,3,1>(convertPointToVector(charge.getLocalPosition()))));
    
        std::map<Pixel::Index, Pulse> pixel_map;

        pixel_map_vector.push_back(pixel_map);

        charge_locations.push_back(charge.getLocalPosition());
        // charge_amounts.push_back(charge.getCharge());
        charge_times.push_back(charge.getLocalTime());
        charge_states.push_back(charge.getState());
    }

    // Continue time propagation until no more objects remain in propagating charge vector 
    Eigen::Vector3d last_position;
    ROOT::Math::XYZVector efield{}, last_efield{};
    size_t next_idx = 0;
    for(time = 0; time < integration_time_; time += timestep_) { // time is the threshold value for each iteration

        if(std::fmod(time, 1) < timestep_){
            LOG(INFO) << "Time during integration has reached " << time << "ns of " << integration_time_ << "ns";
            // LOG(DEBUG) << "Total number of charges skipped due to overlapping positions: " << numSamePos;
        }

        // Loop over all charges remaining in the detector
        for (unsigned int i = 0; i < propagating_charges.size(); i++){
            
            auto &charge = propagating_charges[i];
            auto &runge_kutta = runge_kutta_vector[i]; // Must be a vector in order to keep data from iteration to iteration
            auto position = runge_kutta.getValue();
            // auto state = charge.getState();
            auto type = charge.getType();

            // ROOT::Math::XYZPoint position = charge_locations[i];
            allpix::CarrierState state = charge_states[i];

            if(output_linegraphs_) { // Set final state of charge carrier for plotting:
                auto time_idx = static_cast<size_t>(runge_kutta_vector[i].getTime() / output_plots_step_);
                while(next_idx <= time_idx) {
                    output_plot_points.at(output_plot_index).second.push_back(convertVectorToPoint(position));
                    // output_plot_points.at(output_plot_index).second.push_back(static_cast<ROOT::Math::XYZPoint>(charge.getLocalPosition()));
                    next_idx = output_plot_points.at(output_plot_index).second.size();
                }
            }

            if(runge_kutta.getTime() < time || runge_kutta.getTime() >= time + timestep_){ // Only propagate within a timestep range above the time threshold (time <= rk_time < time + timestep_)
                continue;
            }
            // Now the propagations are calculated only for those in the proper range

            if(state == CarrierState::TRAPPED){ 
                // If it reaches here, it must be within the time range and previously set to trapped. So, we can remove the trapped state and continue propagation
                state = CarrierState::MOTION;
            }else if(state == CarrierState::RECOMBINED || state == CarrierState::HALTED || state == CarrierState::UNKNOWN){
                // I don't think any charges will reach here since they would have to be advanced forward with one of these states.
                continue; // TODO: Check whether I need to finish some other parts of the loop before continuing. Then I need to put the MOTION code in the else clause.
            }
            // At this point, the state must be MOTION and we continue with the propagation

            // Save previous position and time
            last_position = position;
            last_efield = efield;

            // Get electric field at current (pre-step) position
            efield = detector_->getElectricField(convertVectorToPoint(position));
            auto doping = detector_->getDopingConcentration(convertVectorToPoint(position));

            // Execute a Runge Kutta step
            auto step = runge_kutta.step();

            //TODO: Set the charge objects time to the new time so that it can be used in the electric field function for other charges. 
            // It might need to be in a different place so that they don't move out of range (in time) of the other charges.
            charge_times[i]  = runge_kutta.getTime();
            
            // Get the new position due to the electric field
            position = runge_kutta.getValue();

            // Apply diffusion step
            auto diffusion = carrier_diffusion(std::sqrt(efield.Mag2()), doping, timestep_, charge.getType());
            position += diffusion;

            // If charge carrier reaches implant, interpolate surface position for higher accuracy:
            if(auto implant = model_->isWithinImplant(convertVectorToPoint(position))) {
                LOG(TRACE) << "Carrier in implant: " << Units::display(convertVectorToPoint(position), {"nm"});
                auto new_position = model_->getImplantIntercept(implant.value(),
                                                                convertVectorToPoint(last_position),
                                                                convertVectorToPoint(position));
                position = convertPointToVector(new_position);
                state = CarrierState::HALTED;
                // The runge kutta's time will remain at this time
            }

            // Check for overshooting outside the sensor and correct for it:
            if(!model_->isWithinSensor(convertVectorToPoint(position))) {
                // Reflect off the sensor surface with a certain probability, otherwise halt motion:
                if(uniform_distribution(event->getRandomEngine()) > surface_reflectivity_) {
                    LOG(TRACE) << "Carrier outside sensor: "
                            << Units::display(convertVectorToPoint(position), {"nm"});
                    state = CarrierState::HALTED;
                }

                auto intercept = model_->getSensorIntercept(convertVectorToPoint(last_position),
                                                            convertVectorToPoint(position));

                if(state == CarrierState::HALTED) {
                    position = convertPointToVector(intercept);
                } else {
                    // geom. reflection on x-y plane at upper sensor boundary (we have an implant on the lower edge)
                    position = Eigen::Vector3d(position.x(), position.y(), 2. * intercept.z() - position.z());
                    LOG(TRACE) << "Carrier was reflected on the sensor surface to "
                            << Units::display(convertVectorToPoint(position), {"um", "nm"});

                    // Re-check if we ended in an implant - corner case.
                    if(model_->isWithinImplant(convertVectorToPoint(position))) {
                        LOG(TRACE) << "Ended in implant after reflection - halting";
                        state = CarrierState::HALTED;
                    }

                    // Re-check if we are within the sensor - reflection at sensor side walls:
                    if(!model_->isWithinSensor(convertVectorToPoint(position))) {
                        position = convertPointToVector(intercept);
                        state = CarrierState::HALTED;
                    }
                }
                LOG(TRACE) << "Moved carrier to: " << Units::display(convertVectorToPoint(position), {"nm"});
            }

            // Update final position after applying corrections from surface intercepts
            runge_kutta.setValue(position);

            // Update position vector
            charge_locations[i] = convertVectorToPoint(position);

            // Update step length histogram
            if(output_plots_) {
                step_length_histo_->Fill(static_cast<double>(Units::convert(step.value.norm(), "um")));
            }

            // Physics effects:

            // Check if charge carrier is still alive:
            if(recombination_(type, doping, uniform_distribution(event->getRandomEngine()), timestep_)) {
                state = CarrierState::RECOMBINED;
            }
    
            // Check if the charge carrier has been trapped:
            if(trapping_(type, uniform_distribution(event->getRandomEngine()), timestep_, std::sqrt(efield.Mag2()))) {
                state = CarrierState::TRAPPED;
                if(output_plots_) {
                    trapping_time_histo_->Fill(runge_kutta.getTime(), charge.getCharge());
                }
                // Check the detrapping
                auto detrap_time = detrapping_(type, uniform_distribution(event->getRandomEngine()), std::sqrt(efield.Mag2()));
                runge_kutta.advanceTime(detrap_time);

                if((runge_kutta.getTime()) < integration_time_) {
                    
                    // Prepare detrapping here since we have access to detrap_time. The charge will continue to propagate if it is found in the time integration window later on.
                    LOG(TRACE) << "Charge carrier will detrap after " << Units::display(detrap_time, {"ns", "us"});
                    if(output_plots_) {
                        detrapping_time_histo_->Fill(static_cast<double>(Units::convert(detrap_time, "ns")), charge.getCharge());
                    }
                }
            }

            // Don't apply multiplication (charge amounts stay constant)

            // Signal calculation:

            // Find the nearest pixel - before and after the step
            auto [xpixel, ypixel] = model_->getPixelIndex(convertVectorToPoint(position));
            auto [last_xpixel, last_ypixel] = model_->getPixelIndex(convertVectorToPoint(last_position));
            auto idx = Pixel::Index(xpixel, ypixel);
            auto neighbors = model_->getNeighbors(idx, distance_);

            // If the charge carrier crossed pixel boundaries, ensure that we always calculate the induced current for both of
            // them by extending the induction matrix temporarily. Otherwise we end up doing "double-counting" because we would
            // only jump "into" a pixel but never "out". At the border of the induction matrix, this would create an imbalance.
            if(last_xpixel != xpixel || last_ypixel != ypixel) {
                auto last_idx = Pixel::Index(last_xpixel, last_ypixel);
                neighbors.merge(model_->getNeighbors(last_idx, distance_));
                LOG(TRACE) << "Carrier crossed boundary from pixel " << Pixel::Index(last_xpixel, last_ypixel) << " to pixel "
                        << Pixel::Index(xpixel, ypixel);
            }
            LOG(TRACE) << "Moving carriers below pixel " << Pixel::Index(xpixel, ypixel) << " from "
                    << Units::display(convertVectorToPoint(last_position), {"um", "mm"}) << " to "
                    << Units::display(convertVectorToPoint(position), {"um", "mm"}) << ", "
                    << Units::display(runge_kutta.getTime(), "ns");

            for(const auto& pixel_index : neighbors) {
                auto ramo = detector_->getWeightingPotential(convertVectorToPoint(position), pixel_index);
                auto last_ramo = detector_->getWeightingPotential(convertVectorToPoint(last_position), pixel_index);

                // Induced charge on electrode is q_int = q * (phi(x1) - phi(x0))
                auto induced = charge.getCharge() * (ramo - last_ramo) * static_cast<std::underlying_type<CarrierType>::type>(type);

                // This line is commented since we are not applying multiplication
                // auto induced_primary = level != 0 ? 0.
                //                                 : initial_charge * (ramo - last_ramo) *
                //                                         static_cast<std::underlying_type<CarrierType>::type>(type);
                auto induced_primary = induced;
                auto induced_secondary = induced - induced_primary;

                LOG(TRACE) << "Pixel " << pixel_index << " dPhi = " << (ramo - last_ramo) << ", induced " << type
                        << " q = " << Units::display(induced, "e");

                // Create pulse if it doesn't exist. Store induced charge in the returned pulse iterator
                auto pixel_map_iterator = pixel_map_vector[i].emplace(pixel_index, Pulse(timestep_, integration_time_));
                try {
                    pixel_map_iterator.first->second.addCharge(induced, runge_kutta.getTime());
                } catch(const PulseBadAllocException& e) {
                    LOG(ERROR) << e.what() << std::endl
                            << "Ignoring pulse contribution at time "
                            << Units::display(runge_kutta.getTime(), {"ms", "us", "ns"});
                }

                if(output_plots_) {
                    auto inPixel_um_x = (position.x() - model_->getPixelCenter(xpixel, ypixel).x()) * 1e3;
                    auto inPixel_um_y = (position.y() - model_->getPixelCenter(xpixel, ypixel).y()) * 1e3;

                    potential_difference_->Fill(std::fabs(ramo - last_ramo));
                    induced_charge_histo_->Fill(runge_kutta.getTime(), induced);
                    induced_charge_vs_depth_histo_->Fill(runge_kutta.getTime(), position.z(), induced);
                    induced_charge_map_->Fill(inPixel_um_x, inPixel_um_y, induced);
                    if(type == CarrierType::ELECTRON) {
                        induced_charge_e_histo_->Fill(runge_kutta.getTime(), induced);
                        induced_charge_e_vs_depth_histo_->Fill(
                            runge_kutta.getTime(), position.z(), induced);
                        induced_charge_e_map_->Fill(inPixel_um_x, inPixel_um_y, induced);
                    } else {
                        induced_charge_h_histo_->Fill(runge_kutta.getTime(), induced);
                        induced_charge_h_vs_depth_histo_->Fill(
                            runge_kutta.getTime(), position.z(), induced);
                        induced_charge_h_map_->Fill(inPixel_um_x, inPixel_um_y, induced);
                    }
                    if(!multiplication_.is<NoImpactIonization>()) {
                        induced_charge_primary_histo_->Fill(runge_kutta.getTime(), induced_primary);
                        induced_charge_secondary_histo_->Fill(runge_kutta.getTime(), induced_secondary);
                        if(type == CarrierType::ELECTRON) {
                            induced_charge_primary_e_histo_->Fill(runge_kutta.getTime(), induced_primary);
                            induced_charge_secondary_e_histo_->Fill(runge_kutta.getTime(),
                                                                    induced_secondary);
                        } else {
                            induced_charge_primary_h_histo_->Fill(runge_kutta.getTime(), induced_primary);
                            induced_charge_secondary_h_histo_->Fill(runge_kutta.getTime(),
                                                                    induced_secondary);
                        }
                    }
                }
            }
            // Increase charge at the end of the step in case of impact ionization (commented since we are not performing multiplication)
            // charge += n_secondaries;

            // Set the values in vectors to keep them in sync with the propagation
            // TODO: Change the storage of the charges' information such as state and position so that they can be updated
            charge_states[i] = state;

            
        }

    }

    // Add final charges to propagated charges vector
    LOG(INFO) << "Outputing propagated charges";
    for (unsigned int i = 0; i < propagating_charges.size(); i++){

        auto charge = propagating_charges[i];
        auto runge_kutta = runge_kutta_vector[i];

        if(output_linegraphs_) {
            // If drift time is larger than integration time or the charge carriers have been collected at the backside, reset:
            if(runge_kutta.getTime() >= integration_time_ || last_position.z() < -model_->getSensorSize().z() * 0.45) {
                std::get<3>(output_plot_points.at(output_plot_index).first) = CarrierState::UNKNOWN;
            } else {
                std::get<3>(output_plot_points.at(output_plot_index).first) = charge.getState();
            }
        }

        // Create PropagatedCharge object and add it to the list
        auto local_position = static_cast<ROOT::Math::XYZPoint>(runge_kutta.getValue());
        auto global_position = detector_->getGlobalPosition(local_position);
        // LOG(INFO) << "final local: " << local_position << "  final global: " << global_position;
        auto local_time = runge_kutta.getTime();
        auto global_time = local_time - charge.getLocalTime() + charge.getGlobalTime();
        const DepositedCharge* deposit = charge.getDepositedCharge();

        PropagatedCharge propagated_charge(local_position,
                                        global_position,
                                        charge.getType(),
                                        std::move(pixel_map_vector[i]),
                                        local_time,
                                        global_time,
                                        charge.getState(),
                                        deposit);

        LOG(DEBUG) << " Propagated " << charge << " (initial: " << charge.getCharge() << ") to "
                << Units::display(local_position, {"mm", "um"}) << " in " << Units::display(runge_kutta.getTime(), "ns")
                << " time, induced " << Units::display(propagated_charge.getCharge(), {"e"})
                << ", final state: " << allpix::to_string(charge.getState());

        propagated_charges.push_back(std::move(propagated_charge));

        // Calculate the final totals for the recombined, trapped, and propagated charges
        if(charge.getState() == CarrierState::RECOMBINED) {
            recombined_charges_count += charge.getCharge();
            if(output_plots_) {
                recombination_time_histo_->Fill(runge_kutta.getTime(), charge.getCharge());
            }
        } else if(charge.getState() == CarrierState::TRAPPED) { // If the charge still has the TRAPPED state at the integration time, it is clear that the detrapping time was sufficiently large
            trapped_charges_count += charge.getCharge();
        } else {
            propagated_charges_count += charge.getCharge();
        }
    
        if(output_plots_) {
            drift_time_histo_->Fill(static_cast<double>(Units::convert(runge_kutta.getTime(), "ns")), charge.getCharge());
            group_size_histo_->Fill(charge.getCharge());
        }
    }

    return std::make_tuple(recombined_charges_count,trapped_charges_count,propagated_charges_count);
}

// Copied from TransientPropagation.cpp
void InteractivePropagationModule::finalize() {
    LOG(INFO) << deposits_exceeding_max_groups_ * 100.0 / total_deposits_ << "% of deposits have charge exceeding the "
              << max_charge_groups_ << " charge groups allowed, with a charge_per_step value of " << charge_per_step_ << ".";
    if(output_plots_) {
        group_size_histo_->Get()->GetXaxis()->SetRange(1, group_size_histo_->Get()->GetNbinsX() + 1);

        potential_difference_->Write();
        step_length_histo_->Write();
        group_size_histo_->Write();
        drift_time_histo_->Write();
        recombine_histo_->Write();
        recombination_time_histo_->Write();
        trapped_histo_->Write();
        induced_charge_histo_->Write();
        induced_charge_e_histo_->Write();
        induced_charge_h_histo_->Write();
        if(!multiplication_.is<NoImpactIonization>()) {
            induced_charge_primary_histo_->Write();
            induced_charge_primary_e_histo_->Write();
            induced_charge_primary_h_histo_->Write();
            induced_charge_secondary_histo_->Write();
            induced_charge_secondary_e_histo_->Write();
            induced_charge_secondary_h_histo_->Write();
        }
        induced_charge_vs_depth_histo_->Write();
        induced_charge_e_vs_depth_histo_->Write();
        induced_charge_h_vs_depth_histo_->Write();
        induced_charge_map_->Write();
        induced_charge_e_map_->Write();
        induced_charge_h_map_->Write();
        if(!multiplication_.is<NoImpactIonization>()) {
            gain_primary_histo_->Write();
            gain_all_histo_->Write();
            gain_e_histo_->Write();
            gain_h_histo_->Write();
            multiplication_level_histo_->Write();
            multiplication_depth_histo_->Write();
            gain_e_vs_x_->Write();
            gain_e_vs_y_->Write();
            gain_e_vs_z_->Write();
            gain_h_vs_x_->Write();
            gain_h_vs_y_->Write();
            gain_h_vs_z_->Write();
        }
    }
}
