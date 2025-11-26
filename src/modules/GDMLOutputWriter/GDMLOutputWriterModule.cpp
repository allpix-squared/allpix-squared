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
#include <string>

#include <G4GDMLParser.hh>
#include <G4RunManager.hh>

#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

using namespace allpix;

GDMLOutputWriterModule::GDMLOutputWriterModule(Configuration& config, Messenger* /*unused*/, GeometryManager* /*unused*/)
    : Module(config) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
}

void GDMLOutputWriterModule::initialize() {

    std::string const GDML_output_file =
        createOutputFile(config_.get<std::string>("file_name", "Output"), "gdml", false, true);

    G4GDMLParser parser;
    parser.SetRegionExport(true);
    parser.Write(
        GDML_output_file,
        G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking()->GetWorldVolume()->GetLogicalVolume(),
        false);
}
