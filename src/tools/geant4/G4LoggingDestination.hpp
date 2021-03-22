/**
 * @file
 * @brief Set of Geant4 utilities for framework integration
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_GEANT4_LOG_H
#define ALLPIX_GEANT4_LOG_H

#include "core/utils/log.h"

#include <G4UIsession.hh>

namespace allpix {
    class G4LoggingDestination : public G4UIsession {
    public:
        static G4LoggingDestination* getInstance() { return (instance == nullptr ? new G4LoggingDestination() : instance); }

        G4int ReceiveG4cout(const G4String& msg) override;
        G4int ReceiveG4cerr(const G4String& msg) override;

        static void setG4coutReportingLevel(LogLevel level);
        static void setG4cerrReportingLevel(LogLevel level);

    private:
        G4LoggingDestination() = default;

        void process_message(LogLevel level, std::string& msg) const;

        static G4LoggingDestination* instance;
        static LogLevel reporting_level_g4cout;
        static LogLevel reporting_level_g4cerr;
    };

} // namespace allpix

#endif /* ALLPIX_GEANT4_LOG_H */
