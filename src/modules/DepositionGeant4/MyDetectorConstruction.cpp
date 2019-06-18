/**
 * @file
 * @brief Implementation of RunManager
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MyDetectorConstruction.hpp"

#include <G4EmParameters.hh>
#include <G4HadronicProcessStore.hh>
#include <G4LogicalVolume.hh>
#include <G4PhysListFactory.hh>
#include <G4RadioactiveDecayPhysics.hh>
#include <G4StepLimiterPhysics.hh>
#include <G4UImanager.hh>
#include <G4UserLimits.hh>

#include <G4FieldManager.hh>
#include <G4TransportationManager.hh>
#include <G4UniformMagField.hh>

#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "DepositionGeant4Module.hpp"

using namespace allpix;

void MyDetectorConstruction::ConstructSDandField() {
    if(module_->geo_manager_->hasMagneticField()) {
        MagneticFieldType magnetic_field_type_ = module_->geo_manager_->getMagneticFieldType();

        if(magnetic_field_type_ == MagneticFieldType::CONSTANT) {
            ROOT::Math::XYZVector b_field = module_->geo_manager_->getMagneticField(ROOT::Math::XYZPoint(0., 0., 0.));
            G4MagneticField* magField = new G4UniformMagField(G4ThreeVector(b_field.x(), b_field.y(), b_field.z()));
            G4FieldManager* globalFieldMgr = G4TransportationManager::GetTransportationManager()->GetFieldManager();
            globalFieldMgr->SetDetectorField(magField);
            globalFieldMgr->CreateChordFinder(magField);
        } else {
            throw ModuleError("Magnetic field enabled, but not constant. This can't be handled by this module yet.");
        }
    }

    // Loop through all detectors and set the sensitive detector action that handles the particle passage
    for(auto& detector : module_->geo_manager_->getDetectors()) {
        // Do not add sensitive detector for detectors that have no listeners for the deposited charges
        // FIXME Probably the MCParticle has to be checked as well
        // if(!messenger_->hasReceiver(this,
        //                             std::make_shared<DepositedChargeMessage>(std::vector<DepositedCharge>(), detector))) {
        //     LOG(INFO) << "Not depositing charges in " << detector->getName()
        //               << " because there is no listener for its output";
        //     continue;
        // }
        // useful_deposition = true;

        // Get model of the sensitive device
        auto sensitive_detector_action = new SensitiveDetectorActionG4(
            detector, module_->track_info_manager_.get(), charge_creation_energy_, fano_factor_, 4 /* seeder()*/);
        auto logical_volume = detector->getExternalObject<G4LogicalVolume>("sensor_log");
        if(logical_volume == nullptr) {
            throw ModuleError("Detector " + detector->getName() + " has no sensitive device (broken Geant4 geometry)");
        }

        // Apply the user limits to this element
        logical_volume->SetUserLimits(module_->user_limits_.get());

        // Add the sensitive detector action
        logical_volume->SetSensitiveDetector(sensitive_detector_action);
        module_->sensors_.push_back(sensitive_detector_action);

        // // If requested, prepare output plots
        // if(config_.get<bool>("output_plots")) {
        //     LOG(TRACE) << "Creating output plots";

        //     // Plot axis are in kilo electrons - convert from framework units!
        //     int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        //     int nbins = 5 * maximum;

        //     // Create histograms if needed
        //     std::string plot_name = "deposited_charge_" + sensitive_detector_action->getName();
        //     charge_per_event_[sensitive_detector_action->getName()] =
        //         new TH1D(plot_name.c_str(), "deposited charge per event;deposited charge [ke];events", nbins, 0, maximum);
        // }
    }
}
