/**
 * @file
 * @brief Set of Geant4 utilities for framework integration
 *
 * @copyright Copyright (c) 2021-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_GEANT4_EXCEPTIONS_H
#define ALLPIX_GEANT4_EXCEPTIONS_H

#include "core/module/exceptions.h"
#include "core/utils/log.h"

#include <G4VExceptionHandler.hh>

namespace allpix {

    /**
     * @brief Exception handler for Geant4
     *
     * This class is registered with the G4StateManager and handles exceptions thrown in the Geant4 framework. It simply
     * constructs an \ref allpix::ModuleError exception and throws it, for the framework to take further action such as
     * terminating the run
     */
    class G4ExceptionHandler : public G4VExceptionHandler {
    public:
        G4bool Notify(const char*, const char* code, G4ExceptionSeverity severity, const char* description) override {
            std::string message = "Caught Geant4 exception " + std::string(code) + ": " + std::string(description);
            if(severity == G4ExceptionSeverity::JustWarning && std::string(code) != "pl0003") {
                LOG(WARNING) << message;
            } else if(severity == G4ExceptionSeverity::EventMustBeAborted) {
                throw AbortEventException(message);
            } else if(severity == G4ExceptionSeverity::RunMustBeAborted) {
                throw EndOfRunException(message);
            } else {
                throw ModuleError(message);
            }

            // Continue program execution:
            return false;
        };
    };

} // namespace allpix

#endif /* ALLPIX_GEANT4_EXCEPTIONS_H */
