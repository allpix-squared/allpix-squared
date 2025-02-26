/**
 * @file
 * @brief Implementation of Geant4 geometry construction module
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "GDMLOutputWriterModule.hpp"

#include <cassert>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include <G4RunManager.hh>
#include <G4UImanager.hh>
#include <G4UIterminal.hh>
#include <G4Version.hh>
#include <G4VisManager.hh>

#include <Math/Vector3D.h>

#include "tools/ROOT.h"
#include "tools/geant4/geant4.h"

#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"

#include "core/config/ConfigReader.hpp"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "G4GDMLParser.hh"

using namespace allpix;

GDMLOutputWriterModule::GDMLOutputWriterModule(Configuration& config, Messenger*, GeometryManager*) : Module(config) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
}

void GDMLOutputWriterModule::initialize() {

    std::string GDML_output_file = createOutputFile(config_.get<std::string>("file_name", "Output"), "gdml", false, true);

    G4GDMLParser parser;
    parser.SetRegionExport(true);
    parser.Write(
        GDML_output_file,
        G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking()->GetWorldVolume()->GetLogicalVolume(),
        false);
}
