/**
 * @file
 * @brief Set of Geant4 utilities for framework integration
 *
 * @copyright Copyright (c) 2021-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_GEANT4_LOG_H
#define ALLPIX_GEANT4_LOG_H

#include "core/utils/log.h"

#include <G4UIsession.hh>

namespace allpix {

    /**
     * @brief Log message sink for Geant4 output streams
     *
     * This singleton class forwards Geant4 log messages to the framework logger on two configurable verbosity levels, one
     * for the cerr (error) stream, the other for the standard cout stream.
     */
    class G4LoggingDestination : public G4UIsession {
    public:
        /**
         * Method to obtain the singleton object
         * @return G4LoggingDestination instance
         */
        static G4LoggingDestination* getInstance() { return (instance == nullptr ? new G4LoggingDestination() : instance); }

        /**
         * Overloaded G4UIsession method which receives the Geant4 cout stream
         * @param  msg Message with standard cout destiny
         * @return     Zero (success)
         */
        G4int ReceiveG4cout(const G4String& msg) override;

        /**
         * Overloaded G4UIsession method which receives the Geant4 cerr stream
         * @param  msg Message with error cerr destiny
         * @return     Zero (success)
         */
        G4int ReceiveG4cerr(const G4String& msg) override;

        /**
         * Method to set the logger verbosity level for the cout stream
         * @param level Log level
         */
        static void setG4coutReportingLevel(LogLevel level);

        /**
         * Method to set the logger verbosity level for the cerr stream
         * @param level Log level
         */
        static void setG4cerrReportingLevel(LogLevel level);

        /**
         * Method to obtain the current logger verbosity level for the cout stream
         * @return Log level
         */
        static LogLevel getG4coutReportingLevel();

        /**
         * Method to obtain the current logger verbosity level for the cerr stream
         * @return Log level
         */
        static LogLevel getG4cerrReportingLevel();

    private:
        /**
         * Private constructor
         */
        G4LoggingDestination() = default;

        /**
         * Method to process incoming messages from Geant4
         * @param level Log level to decide if message will be forwarded to logger
         * @param msg   Message to be processed
         */
        void process_message(LogLevel level, std::string& msg) const;

        /**
         * Static instance of G4LoggingDestination
         */
        static G4LoggingDestination* instance;

        /**
         * Static log level configurations for cout and cerr
         */
        static LogLevel reporting_level_g4cout;
        static LogLevel reporting_level_g4cerr;
    };

} // namespace allpix

#endif /* ALLPIX_GEANT4_LOG_H */
