/**
 * @file
 * @brief Implementation of Geant4 logging destination
 *
 * @copyright Copyright (c) 2021-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "G4LoggingDestination.hpp"

#include <string>

#include <G4String.hh>
#include <G4Types.hh>

#include "core/utils/log.h"

using namespace allpix;

G4LoggingDestination* G4LoggingDestination::instance = nullptr;
allpix::LogLevel G4LoggingDestination::reporting_level_g4cout = allpix::LogLevel::TRACE;
allpix::LogLevel G4LoggingDestination::reporting_level_g4cerr = allpix::LogLevel::WARNING;

void G4LoggingDestination::setG4coutReportingLevel(LogLevel level) { G4LoggingDestination::reporting_level_g4cout = level; }

void G4LoggingDestination::setG4cerrReportingLevel(LogLevel level) { G4LoggingDestination::reporting_level_g4cerr = level; }

LogLevel G4LoggingDestination::getG4coutReportingLevel() { return G4LoggingDestination::reporting_level_g4cout; }

LogLevel G4LoggingDestination::getG4cerrReportingLevel() { return G4LoggingDestination::reporting_level_g4cerr; }

void G4LoggingDestination::process_message(LogLevel level, std::string& msg) {
    if(!msg.empty() && level <= allpix::Log::getReportingLevel() && !allpix::Log::getStreams().empty()) {
        // Remove line-break always added to G4String
        msg.pop_back();
        auto prev_section = Log::getSection();
        Log::setSection("Geant4");
        allpix::Log().getStream(level, __FILE_NAME__, std::string(static_cast<const char*>(__func__)), __LINE__) << msg;
        Log::setSection(prev_section);
    }
}

G4int G4LoggingDestination::ReceiveG4cout(const G4String& msg) {
    process_message(G4LoggingDestination::reporting_level_g4cout, const_cast<G4String&>(msg)); // NOLINT
    return 0;
}

G4int G4LoggingDestination::ReceiveG4cerr(const G4String& msg) {
    process_message(G4LoggingDestination::reporting_level_g4cerr, const_cast<G4String&>(msg)); // NOLINT
    return 0;
}
