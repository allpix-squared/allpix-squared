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


//This constructor is copied from TransientPropagation.cpp with a few added lines for a few useful constants
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
    config_.setDefault<unsigned int>("charge_per_step", 1);
    config_.setDefault<unsigned int>("max_charge_groups", 1000);
    config_.setDefault<double>("coulomb_distance_limit", Units::get(4e-5,"cm"));
    config_.setDefault<double>("coulomb_field_limit", Units::get(4e5,"V/cm")); // Will need to convert to V/cm to use properly (previously 5760)

    // Models:
    config_.setDefault<std::string>("mobility_model", "jacoboni");
    config_.setDefault<std::string>("recombination_model", "none");
    config_.setDefault<std::string>("trapping_model", "none");
    config_.setDefault<std::string>("detrapping_model", "none");

    config_.setDefault<double>("temperature", 293.15);
    config_.setDefault<unsigned int>("distance", 1);
    config_.setDefault<bool>("ignore_magnetic_field", false);
    config_.setDefault<double>("relative_permittivity", 1.0);
    config_.setDefault<double>("surface_reflectivity", 0.0);

    // Set defaults for charge carrier multiplication (not used currently)
    config_.setDefault<double>("multiplication_threshold", 1e-2);
    config_.setDefault<unsigned int>("max_multiplication_level", 5);
    config_.setDefault<std::string>("multiplication_model", "none");

    // Set defaults for extra configurability
    config_.setDefault<bool>("enable_diffusion", true);
    config_.setDefault<bool>("enable_coulomb_repulsion", true);

    config_.setDefault<bool>("propagate_electrons", true);
    config_.setDefault<bool>("propagate_holes", true);

    config_.setDefault<bool>("include_mirror_charges", false);

    // Set defaults for plots
    config_.setDefault<bool>("output_linegraphs", false);
    config_.setDefault<bool>("output_linegraphs_collected", false);
    config_.setDefault<bool>("output_linegraphs_recombined", false);
    config_.setDefault<bool>("output_linegraphs_trapped", false);
    config_.setDefault<bool>("output_animations", false);
    config_.setDefault<bool>("output_rms", false);
    config_.setDefault<bool>("output_plots",
    config_.get<bool>("output_linegraphs") || config_.get<bool>("output_animations") || config_.get<bool>("output_rms"));
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

    enable_diffusion_ = config.get<bool>("enable_diffusion");
    enable_coulomb_repulsion_ = config.get<bool>("enable_coulomb_repulsion");

    propagate_electrons_ = config.get<bool>("propagate_electrons");
    propagate_holes_ = config.get<bool>("propagate_holes");

    include_mirror_charges_ = config.get<bool>("include_mirror_charges");

    relative_permittivity_ = config.get<double>("relative_permittivity"); // the permativity of materials isn't in allpix, so rely on user to pass this in

    if (enable_coulomb_repulsion_ && relative_permittivity_ == 1) {
        LOG(WARNING) << "Coulomb repulsion is enabled but relative permittivity is set to one. Check that the parameter relative_permittivity isn't misspelled or ommited.";
    }
    
    coulomb_distance_limit_squared_ = config.get<double>("coulomb_distance_limit") * config.get<double>("coulomb_distance_limit") * 1e2; // cm^2 -> mm^2
    coulomb_field_limit_ = config.get<double>("coulomb_field_limit") * 1e-5; // Convert from V/cm to MV/mm (internal field units)
    // coulomb_field_limit_squared_ = config.get<double>("coulomb_field_limit") * config.get<double>("coulomb_field_limit") * 1e-10; // Convert from (V/cm)^2 to (MV/mm)^2 (internal field units)

    output_plots_ = config_.get<bool>("output_plots");
    output_linegraphs_ = config_.get<bool>("output_linegraphs");
    output_linegraphs_collected_ = config_.get<bool>("output_linegraphs_collected");
    output_linegraphs_recombined_ = config_.get<bool>("output_linegraphs_recombined");
    output_linegraphs_trapped_ = config_.get<bool>("output_linegraphs_trapped");
    output_rms_ = config_.get<bool>("output_rms");
    output_plots_step_ = config_.get<double>("output_plots_step");

    // Enable multithreading of this module if multithreading is enabled and no per-event output plots are requested:
    // FIXME: Review if this is really the case or we can still use multithreading
    if(!(config_.get<bool>("output_animations") || output_linegraphs_ || output_rms_)) {
        allow_multithreading();
    } else {
        LOG(WARNING) << "Per-event line graphs or animations requested, disabling parallel event processing";
    }

    // Parameter for charge transport in magnetic field (approximated from graphs:
    // http://www.ioffe.ru/SVA/NSM/Semicond/Si/electric.html) FIXME
    electron_Hall_ = 1.15;
    hole_Hall_ = 0.9;
}

// Copied from TransientPropagation.cpp with one section added to calculate the z-positions of the electrodes
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

    // Apply warnings if things are disabled
    if(!enable_diffusion_){
        LOG(WARNING) << "Diffusion is disabled in propagation. Results will be unphysical.";
    }

    if(!enable_coulomb_repulsion_){
        LOG(WARNING) << "Coulomb Repulsion has been disabled. Use TransientPropagation instead for this use case.";
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

    // Calculate the locations of the upper and lower electrodes.
    // Assumes the local position origin is in the center of the detector
    if (include_mirror_charges_){

        // Determine the distance from the model origin to electrodes in both z-directions
        auto model_size = model_->getSize();
        LOG(DEBUG) << "Model size: " << model_size.x() << ", " << model_size.y() << ", " << model_size.z();
        
        auto model_center = model_->getModelCenter();
        LOG(DEBUG) << "Model center: " << model_center.x() << ", " << model_center.y() << ", " << model_center.z();

        auto sensor_center = model_->getSensorCenter();
        LOG(DEBUG) << "Sensor center: " << sensor_center.x() << ", " << sensor_center.y() << ", " << sensor_center.z();

        auto matrix_center = model_->getMatrixCenter();
        LOG(DEBUG) << "matrix center: " << matrix_center.x() << ", " << matrix_center.y() << ", " << matrix_center.z();

        z_lim_neg_ = model_center.z() - model_size.z() / 2;
        z_lim_pos_ = model_center.z() + model_size.z() / 2;

        //TODO: Correct algorithm to not assume it's in the center
            // z_lim_neg_ = model_->intersect(ROOT::Math::XYZVector(0, 0, -1), ROOT::Math::XYZPoint(0,0,0));
    }

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

        if (output_rms_){
            rms_total_graph_ = new TMultiGraph("rms_total_graph","Comparison of spread of electrons (dashed) and holes (solid);Drift time [ns];RMS [mm]");
            rms_e_subgraph_ = new TGraph();
            rms_e_subgraph_->SetNameTitle("rms_e_subgraph","Spread of electrons");
            rms_e_subgraph_->SetLineColor(kBlack);
            rms_e_subgraph_->SetLineStyle(kDashed);
            rms_h_subgraph_ = new TGraph();
            rms_h_subgraph_->SetNameTitle("rms_h_subgraph","Spread of holes");
            rms_h_subgraph_->SetLineColor(kBlack);
            rms_h_subgraph_->SetLineStyle(kSolid);

            rms_e_graph_ = new TMultiGraph("rms_e_graph","Spread of electrons(xyz=rgb);Drift time [ns];RMS [mm]");
            rms_x_e_subgraph_ = new TGraph();
            rms_x_e_subgraph_->SetNameTitle("rms_x_e_subgraph","Spread in X");
            rms_x_e_subgraph_->SetLineColor(kRed);
            rms_y_e_subgraph_ = new TGraph();
            rms_y_e_subgraph_->SetNameTitle("rms_y_e_subgraph","Spread in Y");
            rms_y_e_subgraph_->SetLineColor(kGreen);
            rms_z_e_subgraph_ = new TGraph();
            rms_z_e_subgraph_->SetNameTitle("rms_z_e_subgraph","Spread in Z");
            rms_z_e_subgraph_->SetLineColor(kBlue);
        }

        coulomb_mag_histo_ =
            CreateHistogram<TH1D>("coulomb_mag_histo",
                      "Direct Coulomb Field Interaction Magnitude;Interaction Field Magnitude [V/cm];Count",
                      200, // Number of bins for the field magnitude
                      0,   // Minimum field magnitude
                      coulomb_field_limit_ * 1e5 // Maximum field magnitude [MV/mm -> V/cm]
            );
    }
}

// Copied from TransientPropagation.cpp with major modifications
void InteractivePropagationModule::run(Event* event) {
    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);

    // Create vector of propagated charges to output
    std::vector<PropagatedCharge> propagated_charges;

    // List of points to plot to plot for output plots
    LineGraph::OutputPlotPoints output_plot_points;

    // Create vector of propagating charges to store each charge groups position, location, time, type, etc. at the start of propagation
    std::vector<PropagatedCharge> propagating_charges;

    //Create vector to temporarily store the applicable deposited charges
    std::vector<DepositedCharge> deposited_charges;

    //Storage of total charge
    unsigned int total_deposited_charge = 0;

    // Initial loop just to get the total charge of applicable charges
    for(const auto& deposit : deposits_message->getData()) {

        // Only process if within requested integration time: 
        if(deposit.getLocalTime() > integration_time_) {
            continue;
        }

        // Skip charges with type not included in propagation
        if(!propagate_electrons_ && deposit.getType() == allpix::CarrierType::ELECTRON){
            continue;
        }
        if(!propagate_holes_ && deposit.getType() == allpix::CarrierType::HOLE){
            continue;
        }

        total_deposits_++;
        total_deposited_charge += deposit.getCharge();

    }

    // The number of charges per charge group based on the total amount of charge
        // In the ideal case of a single deposit, this charge_per_step would give the desired number of charge groups
    auto default_charge_per_step = static_cast<unsigned int>(ceil(static_cast<double>(total_deposited_charge) / std::max(max_charge_groups_,static_cast<unsigned int>(1))));

    // Add the special 0 case where the max_charge_groups parameter is ignored
    if (max_charge_groups_ == 0) {
        default_charge_per_step = charge_per_step_;
    }

    // Choose the maximum charge_per_step value (which gives the minimum number of charge groups)
    auto charge_per_step = charge_per_step_;
    if (default_charge_per_step > charge_per_step) {
        charge_per_step = default_charge_per_step;
        LOG(INFO) << "max_charge_groups = " << max_charge_groups_ << " is the limiting factor in the charge_per_step = " << charge_per_step << " calculation";
    } else {
        LOG(INFO) << "charge_per_step = " << charge_per_step_ << " is the limiting factor in the charge_per_step = " << charge_per_step << " calculation (>" << default_charge_per_step << ")";
    }

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

        // Skip charges with type not included in propagation
        if(!propagate_electrons_ && deposit.getType() == allpix::CarrierType::ELECTRON){
            LOG(DEBUG) << "Skipping "<< deposit.getCharge() <<" electron deposit as per configuration: " << Units::display(deposit.getLocalPosition(), {"mm", "um"});
            continue;
        }
        if(!propagate_holes_ && deposit.getType() == allpix::CarrierType::HOLE){
            LOG(DEBUG) << "Skipping "<< deposit.getCharge() <<" hole deposit as per configuration: " << Units::display(deposit.getLocalPosition(), {"mm", "um"});
            continue;
        }

        LOG(DEBUG) << "Set of "<<deposit.getCharge()<<" charge carriers (" << deposit.getType() << ") on "
                   << Units::display(deposit.getLocalPosition(), {"mm", "um"});
    
        // Split the deposit into charge groups
        unsigned int charges_remaining = deposit.getCharge();
        unsigned int charge_step = charge_per_step; // New charge step variable that gets reset for each deposit
        while(charges_remaining > 0) {

            // Define number of charges to be propagated and remove charges of this step from the total
            if(charge_per_step > charges_remaining) {
                charge_step = charges_remaining; // The final deposit has the remaining charge
            }
            charges_remaining -= charge_step;

            // Add charge to propagating charge vector to be time-stepped later
            PropagatedCharge propagating_charge(deposit.getLocalPosition(),
                                            deposit.getGlobalPosition(),
                                            deposit.getType(),
                                            charge_step,
                                            deposit.getLocalTime(), // The local deposition time
                                            deposit.getGlobalTime(), // The global deposition time
                                            CarrierState::MOTION,
                                            &deposit);

            propagating_charges.push_back(std::move(propagating_charge));
        }
    }
    
    if (propagating_charges.size() > max_charge_groups_){
        LOG(WARNING) << "Number of charge groups (" << propagating_charges.size() << ") exceeded set limit of " << max_charge_groups_ 
            << " due to the large number of deposits with low charge quantity (true limit = set limit + number of deposits)";
    }
    
    LOG(INFO) << "Average number of charges per group is " << total_deposited_charge/propagating_charges.size() << " ("<< propagating_charges.size() <<" total)";
    
    // Propagation occurs within the following function call
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

// This function takes a list of propagating charges to propagate synchronously and places them in the propagated vector
std::tuple<unsigned int, unsigned int, unsigned int>
InteractivePropagationModule::propagate_together(Event* event,
                                                 std::vector<PropagatedCharge>& propagating_charges,
                                                 std::vector<PropagatedCharge>& propagated_charges,
                                                 LineGraph::OutputPlotPoints& output_plot_points) const {

    unsigned int propagated_charges_count = 0;
    unsigned int recombined_charges_count = 0;
    unsigned int trapped_charges_count = 0;

    std::chrono::duration<double, std::nano> time_spent_coulomb{};

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

    // Create vectors that store charge information in a place that can be modified each time step 
        // They need to be here since they are used in dynamic field function, but they are set to initial states below
        // The order of objects within them must stay consistent
    std::vector<ROOT::Math::XYZPoint> charge_locations; // Current position of each charge 
    std::vector<ROOT::Math::XYZPoint> previous_charge_locations; // Positions of each charge at the previous time step (only updated once at the end of each timestep)
    std::vector<double> charge_times; // Most recent time for all of the charges (by the end of propagation they should all be aligned)
    std::vector<allpix::CarrierState> charge_states; // The state of propagation of each charge group (whether it's propagated, trapped, or halted)
    double_t time = 0; // The current time threshold (we only propagate charges near this time)
    int numSamePos = 0; // Counter for debugging the dynamic field collision detection
    unsigned int current_index = 0; // Used for ignoring the current charge from the Coulomb field

    // Computes the coulomb force component of the e-field given a desired local point
    auto coulomb_efield = [&](ROOT::Math::XYZPoint point) -> Eigen::Vector3d {

        auto coulomb_start = std::chrono::system_clock::now();

        ROOT::Math::XYZVector field = ROOT::Math::XYZVector(0,0,0);

        // Predefine some variables to eliminate redefinitions during loop
        ROOT::Math::XYZPoint local_position;
        int q;
        int sign;
        ROOT::Math::XYZVector dist_vector;
        double dist_mag2;
        double dist_mag;
        double interaction_magnitude;

        // Skip function entirely if disabled by configuration file
        if (!enable_coulomb_repulsion_){
            return Eigen::Vector3d(field.x(),field.y(),field.z());
        }

        for (unsigned int i = 0; i < previous_charge_locations.size(); i++){

            // TODO: Add check with (oc)tree object that only looks at charges within a certain distance

            // Only get fields from charges that have deposition time less than the current time (skip the ones that haven't been deposited yet)
            // This means that trapped charges at future times are okay, but not charges that haven't been deposited yet
            // Charges that have reached the sensor (HALTED) are assumed to be swept away and don't contribute to the coulomb field either
            if (propagating_charges[i].getLocalTime() > time || charge_states[i] == allpix::CarrierState::HALTED || charge_states[i] == allpix::CarrierState::RECOMBINED){
                continue;
            }

            // Handling of overlapping charges (that aren't the charge we are calculating for)
            local_position = previous_charge_locations[i];
            if (local_position == point && current_index != i){

                numSamePos += 1;
                
                // Give the overlapping charge a random directional offset so the field at point is in a random direction
                auto phi = uniform_distribution(event->getRandomEngine()) * 2 * ROOT::Math::Pi();
                auto theta = uniform_distribution(event->getRandomEngine()) * ROOT::Math::Pi();
                auto r = ROOT::Math::sqrt(1e-15); // A very small value as to always hit the electric field limit
                auto x = r * ROOT::Math::cos(theta) * ROOT::Math::cos(phi);
                auto y = r * ROOT::Math::cos(theta) * ROOT::Math::sin(phi);
                auto z = r * ROOT::Math::sin(theta);
                local_position = ROOT::Math::XYZPoint(local_position.x() + x, local_position.y() + y, local_position.z() + z);
                // point = ROOT::Math::XYZPoint(point.x() + x, point.y() + y, point.z() + z);
            }

            // Get the correct signed charge
            q = static_cast<int>(propagating_charges[i].getCharge()); // Positive charge [e]
            sign = static_cast<int8_t>(propagating_charges[i].getType()); // Sign of the charge q

            // Calculate the coulomb field due to charges that aren't the current charge
                // The calculation needs to be in the if-statement rather than a terminiation/continue since we still want to include the mirror charges of the current charge
            if (current_index != i){
            
                dist_vector = point - local_position; // A vector between the desired points (mm)
                dist_mag2 =  dist_vector.Mag2();

                if (dist_mag2 < coulomb_distance_limit_squared_){ // Limit the following calculations to if the distance of the charge is close enough to give a significant field

                    dist_mag = ROOT::Math::sqrt(dist_mag2);

                    interaction_magnitude = std::min(coulomb_field_limit_, coulomb_K_ / relative_permittivity_ * q / dist_mag2); // Magnitude of the force [MV/mm] (always positive)
                    coulomb_mag_histo_ -> Fill(interaction_magnitude * 1e5); // Conversion from MV/mm to V/cm
                    
                    // Add this charge's field to the total field at the point
                    field = field + dist_vector/dist_mag * sign * interaction_magnitude; 

                }
            }

            // Skip mirror charges when specified
            if (!include_mirror_charges_){
                continue;
            }

            // Perform same for the mirror charges based on electrode positions (z_lim_neg and z_lim_pos)
                // Note: this assumes a parallel plate sensor (symmetry about z) in order to simplify the poisson equation to the mirror charge solution 
                // (potential is constant on each plate)
            ROOT::Math::XYZPoint mirror_position_neg = ROOT::Math::XYZPoint(local_position.x(), local_position.y(), 2*z_lim_neg_ - local_position.z());
            ROOT::Math::XYZPoint mirror_position_pos = ROOT::Math::XYZPoint(local_position.x(), local_position.y(), 2*z_lim_pos_ - local_position.z());

            // Apply field for negative-side mirror charge
            dist_vector = point - mirror_position_neg;
            dist_mag2 = dist_vector.Mag2();

            if (dist_mag2 < coulomb_distance_limit_squared_){
                dist_mag = ROOT::Math::sqrt(dist_mag2);
                field = field - dist_vector/dist_mag * sign * std::min(coulomb_field_limit_, coulomb_K_ / relative_permittivity_ * q / dist_mag2); // Mirror charges have opposite charge
            }

            // Apply field for positive-side mirror charge
            dist_vector = point - mirror_position_pos;
            dist_mag2 = dist_vector.Mag2();

            if (dist_mag2 < coulomb_distance_limit_squared_){
                dist_mag = ROOT::Math::sqrt(dist_mag2);
                field = field - dist_vector/dist_mag * sign * std::min(coulomb_field_limit_, coulomb_K_ / relative_permittivity_ * q / dist_mag2);
            }
        }

        // TODO: Rather than using coulomb_field_limit for each interaction, we could use it on the final value. 
            // (commented out for now since determining a good value is tricky)
            // if (field.mag2() > coulomb_field_limit_squared_){
            //     double field_mag = ROOT::Math::sqrt(field.mag2());
            //     // LOG(INFO) << "Skipping field with magnitude " << field_mag << " > "<<coulomb_field_limit_;
            //     field = Eigen::Vector3d(field.x() / field_mag * coulomb_field_limit_, field.y() / field_mag * coulomb_field_limit_, field.z() / field_mag * coulomb_field_limit_);
            //     // LOG(INFO) << "   now " << field.mag2();
            //     numSamePos+=1;
            // }

        Eigen::Vector3d output = Eigen::Vector3d(field.x(),field.y(),field.z());

        auto coulomb_end = std::chrono::system_clock::now();

        time_spent_coulomb += coulomb_end - coulomb_start;

        return output;
    };

    // Define lambda functions to compute the charge carrier velocity with or without magnetic field
    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&, allpix::CarrierType type)> carrier_velocity_noB =
        [&](double, const Eigen::Vector3d& cur_pos, allpix::CarrierType type) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z());

        efield = efield + coulomb_efield(static_cast<ROOT::Math::XYZPoint>(cur_pos)); // Includes the dynamic field from charge interaction

        auto doping = detector_->getDopingConcentration(static_cast<ROOT::Math::XYZPoint>(cur_pos));

        return static_cast<int>(type) * mobility_(type, efield.norm(), doping) * efield;
    };

    std::function<Eigen::Vector3d(double, const Eigen::Vector3d&, allpix::CarrierType type)> carrier_velocity_withB =
        [&](double, const Eigen::Vector3d& cur_pos, allpix::CarrierType type) -> Eigen::Vector3d {
        auto raw_field = detector_->getElectricField(static_cast<ROOT::Math::XYZPoint>(cur_pos));
        Eigen::Vector3d efield(raw_field.x(), raw_field.y(), raw_field.z()); 

        efield = efield + coulomb_efield(static_cast<ROOT::Math::XYZPoint>(cur_pos)); // Includes the dynamic field from charge interaction

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

    // Helper functions that convert between ROOT::Math:XYZPoint and Eigen::Vector3d (Eigen::Matrix<double, 3, 1>)
    auto convertPointToVector = [] (ROOT::Math::XYZPoint point) -> Eigen::Vector3d {
        return Eigen::Vector3d(point.x(), point.y(), point.z());
    };

    auto convertVectorToPoint = [] (Eigen::Vector3d vector) -> ROOT::Math::XYZPoint {
        return ROOT::Math::XYZPoint(vector.x(), vector.y(), vector.z());
    };

    auto convertRootVectorToEigenVector = [] (ROOT::Math::XYZVector vector) -> Eigen::Vector3d {
        return Eigen::Vector3d(vector.x(), vector.y(), vector.z());
    };

    // Create the pixel map which is used to collect the pulse objects
    std::vector< std::map<Pixel::Index, Pulse> > pixel_map_vector;

    // Create list of RK4 objects that correspond to each particle
    std::vector<allpix::RungeKutta<double, 4, 3>> runge_kutta_vector;

    // Initialize all of the vectors with their starting values from each charge group
    for(auto charge : propagating_charges){
        
        // Specify the charge type before passing into the rungekutta 
            // the rk step function can only have two arguments: t and pos
        std::function<Eigen::Matrix<double, 3, 1>(double, Eigen::Matrix<double, 3, 1>)> step_function;
        if (has_magnetic_field_){
            step_function = [=](double t, Eigen::Vector3d pos) -> Eigen::Vector3d {return carrier_velocity_withB(t, pos, charge.getType());};
        }else{
            step_function = [=](double t, Eigen::Vector3d pos) -> Eigen::Vector3d {return carrier_velocity_noB(t, pos, charge.getType());};
        }

        auto rk = make_runge_kutta(
            tableau::RK4, 
            step_function,
            timestep_, 
            convertPointToVector(charge.getLocalPosition())); // No error estimation required since we're not adapting step size

        // Set the start time of each to the local time of the charges deposition
        rk.advanceTime(charge.getLocalTime());
                                       
        // Fill the vectors with their starting values for the current charge
        runge_kutta_vector.push_back(rk);
        std::map<Pixel::Index, Pulse> pixel_map; // Pixel map is required for pulse
        pixel_map_vector.push_back(pixel_map);
        charge_locations.push_back(charge.getLocalPosition());
        previous_charge_locations.push_back(charge.getLocalPosition());
        charge_times.push_back(charge.getLocalTime());
        charge_states.push_back(charge.getState());

        // Add point of deposition to the output plots if requested
        if(output_linegraphs_) {
            output_plot_points.emplace_back(std::make_tuple(charge.getGlobalTime(), charge.getCharge(), charge.getType(), CarrierState::MOTION),
                                            std::vector<ROOT::Math::XYZPoint>());
        }

    }

    // Set up variables that are changed each loop
    Eigen::Vector3d efield{};
    allpix::PropagatedCharge &charge = propagating_charges[0];
    ROOT::Math::XYZPoint position{}; // = ROOT::Math::XYZPoint();
    ROOT::Math::XYZPoint previous_position{}; // = ROOT::Math::XYZPoint();
    allpix::CarrierType type = charge.getType();
    allpix::CarrierState state{};

    // Continue time propagation until the integration time has been reached
    for(time = 0; time < integration_time_; time += timestep_) { // time is the threshold value for each iteration

        // Based on the desired output_plots_step, display integration progress and calculate rms if desired
        if(std::fmod(time, output_plots_step_) < timestep_){
            // TODO: Change output_plots_step implementation to not depend on floating point errors.

            // This code could be useful in making the output_plots_step more consistent and not dependent on floating point errors
                // auto time_idx = static_cast<size_t>(runge_kutta_vector[i].getTime() / output_plots_step_);
                // while(next_idx <= time_idx) {
                    
                //     // output_plot_points.at(output_plot_index).second.push_back(static_cast<ROOT::Math::XYZPoint>(charge.getLocalPosition()));
                //     next_idx = output_plot_points.at(i).second.size();
                // }

            LOG(DEBUG) << "Time has reached " << time << "ns of " << integration_time_ << "ns";

            for (unsigned int i = 0; i < propagating_charges.size(); i++){
                // Add the current position to the linegraph associated with the current charge
                
            }

            // Get RMS of the charge distribution
            if (output_rms_){

                // Start by calculating the mean
                double x_mean_e = 0; double y_mean_e = 0; double z_mean_e = 0;
                double x_mean_h = 0; double y_mean_h = 0; double z_mean_h = 0;
                
                double num_e = 0; 
                double num_h = 0;
                for (unsigned int i = 0; i < charge_locations.size(); i++){
                    auto location = charge_locations[i];

                    // TODO: Think about whether there are certain states or time conditions we want to remove from RMS calc (ie RECOMBINED)

                    if (propagating_charges[i].getType() == allpix::CarrierType::ELECTRON){
                        num_e++;
                        x_mean_e += location.x();
                        y_mean_e += location.y();
                        z_mean_e += location.z();
                    }else{
                        num_h++;
                        x_mean_h += location.x();
                        y_mean_h += location.y();
                        z_mean_h += location.z();
                    }
                }

                if (num_e > 0){
                    x_mean_e = x_mean_e/num_e;
                    y_mean_e = y_mean_e/num_e;
                    z_mean_e = z_mean_e/num_e;
                }
                if (num_h > 0){
                    x_mean_h = x_mean_h/num_h;
                    y_mean_h = y_mean_h/num_h;
                    z_mean_h = z_mean_h/num_h;
                }
                
                // Now sum the square of the residuals (split up into x, y and z)
                double res2_x_e = 0; double res2_y_e = 0; double res2_z_e = 0;
                double res2_x_h = 0; double res2_y_h = 0; double res2_z_h = 0;

                for (unsigned int i = 0; i < charge_locations.size(); i++){

                    auto location = charge_locations[i];

                    if (propagating_charges[i].getType() == allpix::CarrierType::ELECTRON){
                        res2_x_e += (location.x() - x_mean_e)*(location.x() - x_mean_e);
                        res2_y_e += (location.y() - y_mean_e)*(location.y() - y_mean_e);
                        res2_z_e += (location.z() - z_mean_e)*(location.z() - z_mean_e);
                    }else{
                        res2_x_h += (location.x() - x_mean_h)*(location.x() - x_mean_h);
                        res2_y_h += (location.y() - y_mean_h)*(location.y() - y_mean_h);
                        res2_z_h += (location.z() - z_mean_h)*(location.z() - z_mean_h);
                    }
                }

                // Divide by the total number of charges of each type
                double rms_total_e = 0; double rms_x_e = 0; double rms_y_e = 0; double rms_z_e = 0;
                if (num_e > 0){
                    rms_total_e = sqrt((res2_x_e + res2_y_e + res2_z_e)/num_e);
                    rms_x_e = sqrt(res2_x_e/num_e);
                    rms_y_e = sqrt(res2_y_e/num_e);
                    rms_z_e = sqrt(res2_z_e/num_e);
                }
                double rms_total_h = 0; // double rms_x_h = 0; double rms_y_h = 0; double rms_z_h = 0;
                if (num_h > 0){
                    rms_total_h = sqrt((res2_x_h + res2_y_h + res2_z_h)/num_h);
                    // double rms_x_h = sqrt(res2_x_h/num_h); // Holes are less important, so ignore the separation of axes
                    // double rms_y_h = sqrt(res2_y_h/num_h);
                    // double rms_z_h = sqrt(res2_z_h/num_h);
                }

                // Add to ROOT Graphs
                rms_x_e_subgraph_->AddPoint(time, rms_x_e);
                rms_y_e_subgraph_->AddPoint(time, rms_y_e);
                rms_z_e_subgraph_->AddPoint(time, rms_z_e);
                rms_e_subgraph_->AddPoint(time, rms_total_e);
                rms_h_subgraph_->AddPoint(time, rms_total_h);

            }
        }

        // Copy the current positions to the previous positions
        for (unsigned int i = 0; i < charge_locations.size(); i++){
            previous_charge_locations[i] = charge_locations[i];   
        }

        // Move all charges by a single timestep
        for (unsigned int i = 0; i < propagating_charges.size(); i++){
            
            // Update local variables for convenient access and reduced array calling
            auto &runge_kutta = runge_kutta_vector[i];
            position = convertVectorToPoint(runge_kutta.getValue());
            state = charge_states[i];

            // TODO: Change output_plots_step implementation to not depend on floating point errors.
            if(output_linegraphs_ && std::fmod(time, output_plots_step_) < timestep_) {
                output_plot_points.at(i).second.push_back(position);
            }

            // Only propagate within a timestep range above the time threshold (time <= rk_time < time + timestep_)
            if(runge_kutta.getTime() < time || runge_kutta.getTime() >= time + timestep_){ 
                continue;
            }
            // Now the propagations are calculated only for those in the proper range

            if(state == CarrierState::TRAPPED){ 
                // If it reaches here, it must be within the time range and previously set to trapped. So, we can remove the trapped state and continue propagation
                state = CarrierState::MOTION;
            }else if(state == CarrierState::RECOMBINED || state == CarrierState::HALTED || state == CarrierState::UNKNOWN){
                // I don't think any charges will reach here since they would have to be advanced forward with one of these states.
                continue;
            }
            // At this point, the state must be MOTION and we continue with the propagation

            // Update more local variables that aren't needed above (saves this for after the time and state filtering)
            charge = propagating_charges[i];
            previous_position = previous_charge_locations[i];
            type = charge.getType();
            current_index = i; // Update for use in dynamic_field

            // Get electric field at current (pre-step) position
            // TODO: add a storage of the dynamic field so that we don't have to calculate it an extra time for use in diffusion
            efield = convertRootVectorToEigenVector(detector_->getElectricField(position));
            efield += coulomb_efield(position);
            auto doping = detector_->getDopingConcentration(position); //TODO: Does doping affect the dynamic field at all?

            // Execute a Runge Kutta step and update time in the vector
            auto step = runge_kutta.step();
            charge_times[i]  = runge_kutta.getTime();
            
            // Get the new position due to the electric field
            position = convertVectorToPoint(runge_kutta.getValue());

            // Apply diffusion step (if enabled)
            if (enable_diffusion_){
                auto diffusion = carrier_diffusion(efield.norm(), doping, timestep_, charge.getType());
                position = ROOT::Math::XYZPoint(position.x() + diffusion.x(), position.y() + diffusion.y(), position.z() + diffusion.z());
            }

            // If charge carrier reaches implant, interpolate surface position for higher accuracy:
            if(auto implant = model_->isWithinImplant(position)) {
                LOG(TRACE) << "Carrier in implant: " << Units::display(position, {"nm"});
                auto new_position = model_->getImplantIntercept(implant.value(), previous_position, position);
                position = convertPointToVector(new_position);
                state = CarrierState::HALTED;
                // The runge kutta's time will remain at the time that this gets triggered
            }

            // Check for overshooting outside the sensor and correct for it:
            if(!model_->isWithinSensor(position)) {
                // Reflect off the sensor surface with a certain probability, otherwise halt motion:
                if(uniform_distribution(event->getRandomEngine()) > surface_reflectivity_) {
                    LOG(TRACE) << "Carrier outside sensor: "
                            << Units::display(position, {"nm"});
                    state = CarrierState::HALTED;
                }

                auto intercept = model_->getSensorIntercept(previous_position, position);

                if(state == CarrierState::HALTED) {
                    position = intercept;
                } else {
                    // geom. reflection on x-y plane at upper sensor boundary (we have an implant on the lower edge)
                    position = ROOT::Math::XYZPoint(position.x(), position.y(), 2. * intercept.z() - position.z());
                    LOG(TRACE) << "Carrier was reflected on the sensor surface to "
                            << Units::display(position, {"um", "nm"});

                    // Re-check if we ended in an implant - corner case.
                    if(model_->isWithinImplant(position)) {
                        LOG(TRACE) << "Ended in implant after reflection - halting";
                        state = CarrierState::HALTED;
                    }

                    // Re-check if we are within the sensor - reflection at sensor side walls:
                    if(!model_->isWithinSensor(position)) {
                        position = intercept;
                        state = CarrierState::HALTED;
                    }
                }
                LOG(TRACE) << "Moved carrier to: " << Units::display(position, {"nm"});
            }

            // Update final position after applying corrections from surface intercepts
            runge_kutta.setValue(convertPointToVector(position));

            // Update position vector after e-field and diffusion so it is up to date in in dynamic field calculation
            charge_locations[i] = position;

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
            if(trapping_(type, uniform_distribution(event->getRandomEngine()), timestep_, efield.norm())) {
                state = CarrierState::TRAPPED;
                if(output_plots_) {
                    trapping_time_histo_->Fill(runge_kutta.getTime(), charge.getCharge());
                }
                // Check the detrapping
                auto detrap_time = detrapping_(type, uniform_distribution(event->getRandomEngine()), efield.norm());
                runge_kutta.advanceTime(detrap_time);

                if((runge_kutta.getTime()) < integration_time_) {
                    
                    // Prepare detrapping here since we have access to detrap_time. The charge will continue to propagate if it is found in the time integration window later on.
                    LOG(TRACE) << "Charge carrier will detrap after " << Units::display(detrap_time, {"ns", "us"});
                    if(output_plots_) {
                        detrapping_time_histo_->Fill(static_cast<double>(Units::convert(detrap_time, "ns")), charge.getCharge());
                    }
                }
            }

            // No multiplication occurs since adding more charge groups increases simulation time dramatically

            // Signal calculation:

            // Find the nearest pixel - before and after the step
            auto [xpixel, ypixel] = model_->getPixelIndex(position);
            auto [last_xpixel, last_ypixel] = model_->getPixelIndex(previous_position);
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
                    << Units::display(previous_position, {"um", "mm"}) << " to "
                    << Units::display(position, {"um", "mm"}) << ", "
                    << Units::display(runge_kutta.getTime(), "ns");

            for(const auto& pixel_index : neighbors) {
                auto ramo = detector_->getWeightingPotential(position, pixel_index);
                auto last_ramo = detector_->getWeightingPotential(previous_position, pixel_index);

                // Induced charge on electrode is q_int = q * (phi(x1) - phi(x0))
                auto induced = charge.getCharge() * (ramo - last_ramo) * static_cast<std::underlying_type<CarrierType>::type>(type);

                // This line is commented since we are not applying multiplication
                // auto induced_primary = level != 0 ? 0.
                //                                 : initial_charge * (ramo - last_ramo) *
                //                                         static_cast<std::underlying_type<CarrierType>::type>(type);
                auto induced_primary = induced;
                auto induced_secondary = induced - induced_primary; // TODO: If multiplocation isn't reimplemented, remove the redundant info

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
                    if(!multiplication_.is<NoImpactIonization>()) { //TODO: If muliplication isn't reimplemented, remove the primary and secondary histogram
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
            charge_states[i] = state;

        }

    }

    // Add final charges to propagated charges vector
    LOG(INFO) << "Outputing propagated charges";
    for (unsigned int i = 0; i < propagating_charges.size(); i++){

        charge = propagating_charges[i];
        auto runge_kutta = runge_kutta_vector[i];

        if(output_linegraphs_) {
            std::get<3>(output_plot_points.at(i).first) = charge_states[i];
        }

        // Create PropagatedCharge object and add it to the list
        auto local_position = convertVectorToPoint(runge_kutta.getValue());
        auto global_position = detector_->getGlobalPosition(local_position);
        auto local_time = runge_kutta.getTime();
        auto global_time = local_time - charge.getLocalTime() + charge.getGlobalTime();

        const DepositedCharge* deposit = charge.getDepositedCharge();

        PropagatedCharge propagated_charge(local_position,
                                        global_position,
                                        charge.getType(),
                                        std::move(pixel_map_vector[i]),
                                        local_time,
                                        global_time,
                                        charge_states[i],
                                        deposit);

        LOG(DEBUG) << " Propagated " << charge << " (initial: " << charge.getCharge() << ") to "
                << Units::display(local_position, {"mm", "um"}) << " in " << Units::display(runge_kutta.getTime(), "ns")
                << " time, induced " << Units::display(propagated_charge.getCharge(), {"e"})
                << ", final state: " << allpix::to_string(charge_states[i]);

        propagated_charges.push_back(std::move(propagated_charge));

        // Calculate the final totals for the recombined, trapped, and propagated charges
        
        if(charge_states[i] == CarrierState::RECOMBINED) {
            recombined_charges_count += charge.getCharge();
            if(output_plots_) {
                recombination_time_histo_->Fill(runge_kutta.getTime(), charge.getCharge());
            }
        } else if(charge_states[i] == CarrierState::TRAPPED) { // If the charge still has the TRAPPED state at the integration time, it is clear that the detrapping time was sufficiently large
            trapped_charges_count += charge.getCharge();
        } else {
            propagated_charges_count += charge.getCharge();
        }
    
        if(output_plots_) {
            drift_time_histo_->Fill(static_cast<double>(Units::convert(runge_kutta.getTime(), "ns")), charge.getCharge()); //TODO: Check whether we need to remove the "dead time" before deposition
            group_size_histo_->Fill(charge.getCharge());
        }
    }

    LOG(INFO) << "The running of the coulomb_efield function took a combined " << time_spent_coulomb.count()/1e6 << "ms";

    return std::make_tuple(recombined_charges_count,trapped_charges_count,propagated_charges_count);
}

// Copied from TransientPropagation.cpp with addition of rms plots
void InteractivePropagationModule::finalize() {
    LOG(INFO) << deposits_exceeding_max_groups_ * 100.0 / total_deposits_ << "% of deposits have charge exceeding the "
              << max_charge_groups_ << " charge groups allowed, with a charge_per_step value of " << charge_per_step_ << "."; //TODO: Change to make sense with the new interpretation of max_charge_groups
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

        if (output_rms_){
            rms_total_graph_->Add(rms_e_subgraph_);
            rms_total_graph_->Add(rms_h_subgraph_);

            rms_e_graph_->Add(rms_x_e_subgraph_);
            rms_e_graph_->Add(rms_y_e_subgraph_);
            rms_e_graph_->Add(rms_z_e_subgraph_);
            rms_e_graph_->Add(rms_e_subgraph_);

            rms_total_graph_->Write();
            rms_e_graph_->Write();
        }

        coulomb_mag_histo_->Write();
    }
}
