/**
 * @file
 * @brief Implementation of configuration exceptions
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "core/config/exceptions.h"
#include "Configuration.hpp"
#include "core/module/ModuleIdentifier.hpp"

using namespace allpix;

InvalidValueError::InvalidValueError(const Configuration& config, const std::string& key, const std::string& reason) {
    std::string section_str = "in section '" + config.getName() + "'";
    if(config.getName().empty()) {
        section_str = "in global section";
    }
    error_message_ = "Value " + config.getText(key) + " of key '" + key + "' " + section_str + " is not valid";
    if(!reason.empty()) {
        error_message_ += ": " + reason;
    }
}

InvalidCombinationError::InvalidCombinationError(const Configuration& config,
                                                 std::initializer_list<std::string> keys,
                                                 const std::string& reason) {
    std::string section_str = "in section '" + config.getName() + "'";
    if(config.getName().empty()) {
        section_str = "in global section";
    }
    error_message_ = "Combination of keys ";
    for(const auto& key : keys) {
        if(!config.has(key)) {
            continue;
        }
        error_message_ += "'" + key + "', ";
    }
    error_message_ += section_str + " is not valid";
    if(!reason.empty()) {
        error_message_ += ": " + reason;
    }
}

ModuleIdentifierNotFoundError::ModuleIdentifierNotFoundError(const ModuleIdentifier& identifier) {
    error_message_ = "Module Identifier " + identifier.getUniqueName() + ":" + std::to_string(identifier.getPriority()) +
                     " not found in the module identifier list";
}

ModuleIdentifierAlreadyAddedError::ModuleIdentifierAlreadyAddedError(const ModuleIdentifier& identifier) {
    error_message_ = "Module Identifier " + identifier.getUniqueName() + ":" + std::to_string(identifier.getPriority()) +
                     " already added to the module identifier list";
}
