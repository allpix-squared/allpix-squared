/**
 * @file
 * @brief Implementation of module identifier
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <string>
#include <vector>

#include "core/module/ModuleIdentifier.hpp"

using namespace allpix;

ModuleIdentifier::ModuleIdentifier(const Configuration& config, int prio) : name_(config.getName()), prio_(prio) {

    // Create the identifier
    if(!config.get<std::string>("input").empty()) {
        identifier_ += config.get<std::string>("input");
    }
    if(!config.get<std::string>("output").empty()) {
        if(!identifier_.empty()) {
            identifier_ += "_";
        }
        identifier_ += config.get<std::string>("output");
    }
}

ModuleIdentifier::ModuleIdentifier(const Configuration& config, const std::string& detector, int prio)
    : ModuleIdentifier(config, prio) {

    // Prefix the detector to the identifier
    identifier_ = detector + (identifier_.empty() ? "" : "_") + identifier_;
}

std::string ModuleIdentifier::getUniqueName() const {
    std::string unique_name = name_;
    if(!identifier_.empty()) {
        unique_name += ":" + identifier_;
    }
    return unique_name;
}
