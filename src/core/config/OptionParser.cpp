/**
 * @file
 * @brief Implementation of option parser
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "OptionParser.hpp"

#include "ConfigReader.hpp"
#include "core/utils/log.h"

using namespace allpix;

/**
 * Option is split in a key / value pair, an error is thrown if that is not possible. When the key contains at least one dot
 * it is interpreted as a relative configuration with the module / detector identified by the first dot. In that case the
 * option is applied during loading when either the unique or the configuration name match. Otherwise the key is interpreted
 * as global key and is added to the global header.
 */
void OptionParser::parseOption(std::string line) {
    line = allpix::trim(line);
    auto [key, value] = ConfigReader::parseKeyValue(std::move(line));

    auto dot_pos = key.find('.');
    if(dot_pos == std::string::npos) {
        // Global option, add to the global options list
        global_options_.emplace_back(key, value);
    } else {
        // Other identifier bound option is passed
        auto identifier = key.substr(0, dot_pos);
        key = key.substr(dot_pos + 1);
        identifier_options_[identifier].emplace_back(key, value);
    }
}

bool OptionParser::applyGlobalOptions(Configuration& config) {
    for(auto& [key, value] : global_options_) {
        LOG(INFO) << "Setting provided option " << key << '=' << value;
        config.setText(key, value);
    }
    return !global_options_.empty();
}

bool OptionParser::applyOptions(const std::string& identifier, Configuration& config) {
    if(identifier_options_.find(identifier) == identifier_options_.end()) {
        return false;
    }

    for(auto& [key, value] : identifier_options_[identifier]) {
        LOG(INFO) << "Setting provided option " << key << '=' << value << " for " << identifier;
        config.setText(key, value);
    }
    return true;
}
