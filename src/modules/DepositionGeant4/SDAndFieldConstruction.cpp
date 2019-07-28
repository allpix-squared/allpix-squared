/**
 * @file
 * @brief Implementation of SDAndFieldConstruction
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "SDAndFieldConstruction.hpp"

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

#include "DepositionGeant4Module.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"

using namespace allpix;

void SDAndFieldConstruction::ConstructSDandField() {
    module_->construct_sensitive_detectors_and_fields(fano_factor_, charge_creation_energy_);
}
