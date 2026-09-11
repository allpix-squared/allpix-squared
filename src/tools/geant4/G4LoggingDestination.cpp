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

#include "core/log/LoggerManager.hpp"
#include "core/utils/log.h"

using namespace allpix;

G4LoggingDestination* G4LoggingDestination::instance = nullptr;
allpix::LogLevel G4LoggingDestination::reporting_level_g4cout = allpix::LogLevel::TRACE;
allpix::LogLevel G4LoggingDestination::reporting_level_g4cerr = allpix::LogLevel::WARNING;

void G4LoggingDestination::setG4coutReportingLevel(LogLevel level) { G4LoggingDestination::reporting_level_g4cout = level; }

void G4LoggingDestination::setG4cerrReportingLevel(LogLevel level) { G4LoggingDestination::reporting_level_g4cerr = level; }

LogLevel G4LoggingDestination::getG4coutReportingLevel() { return G4LoggingDestination::reporting_level_g4cout; }

LogLevel G4LoggingDestination::getG4cerrReportingLevel() { return G4LoggingDestination::reporting_level_g4cerr; }

namespace {
    /**
     * @brief This module's own permanent logger (topic "Geant4"), seeded from the global default level/format the first
     * time a Geant4 message is received
     */
    Logger& geant4_logger() { return LoggerManager::getInstance().getLogger("Geant4"); }
} // namespace

void G4LoggingDestination::setG4ReportingLevel(LogLevel level) { geant4_logger().setLevel(level); }

void G4LoggingDestination::process_message(LogLevel level, std::string& msg) {
    if(msg.empty()) {
        return;
    }

    auto& logger = geant4_logger();
    if(!logger.shouldLog(level)) {
        return;
    }
    // Remove line-break always added to G4String
    msg.pop_back();
    logger.log(level) << msg;
}

G4int G4LoggingDestination::ReceiveG4cout(const G4String& msg) {
    process_message(G4LoggingDestination::reporting_level_g4cout, const_cast<G4String&>(msg)); // NOLINT
    return 0;
}

G4int G4LoggingDestination::ReceiveG4cerr(const G4String& msg) {
    process_message(G4LoggingDestination::reporting_level_g4cerr, const_cast<G4String&>(msg)); // NOLINT
    return 0;
}
