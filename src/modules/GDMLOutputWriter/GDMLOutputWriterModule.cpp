/**
 * @file
 * @brief Implementation of Geant4 geometry construction module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "GDMLOutputWriterModule.hpp"

#include <cassert>
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
#include "tools/geant4.h"

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"

#include "core/config/ConfigReader.hpp"
#include "core/utils/log.h"

// Include GDML if Geant4 version has it
#ifdef Geant4_GDML
#include "G4GDMLParser.hh"
#endif

using namespace allpix;
using namespace ROOT;

GDMLOutputWriterModule::GDMLOutputWriterModule(Configuration& config, Messenger*, GeometryManager*) : Module(config) {}

void GDMLOutputWriterModule::init() {

    SUPPRESS_STREAM(G4cout);

// Export geometry in GDML if requested (and GDML support is available in Geant4)
#ifdef Geant4_GDML
    auto file_name = config_.get<std::string>("file_name", "Output");
    auto output_file = createOutputFile(file_name);
    output_file += ".gdml";
    G4GDMLParser parser;
    parser.SetRegionExport(true);
    parser.Write(output_file,
                 G4TransportationManager::GetTransportationManager()
                     ->GetNavigatorForTracking()
                     ->GetWorldVolume()
                     ->GetLogicalVolume());
#else
    std::string error = "You requested to export the geometry in GDML. ";
    error += "However, GDML support is currently disabled in Geant4. ";
    error += "To enable it, configure and compile Geant4 with the option -DGEANT4_USE_GDML=ON.";
    throw allpix::InvalidValueError(config_, "GDML_output_file_name", error);
#endif

    // Release output from G4
    RELEASE_STREAM(G4cout);
}
