/**
 * @file
 * @brief Implementation of string utilities
 *
 * @copyright Copyright (c) 2016-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Used extensively for parsing the configuration in the \ref allpix::ConfigReader.
 */

#include "text.h"

#include "unit.h"

using namespace allpix;

std::string allpix::trim(const std::string& str, const std::string& delims) {
    size_t b = str.find_first_not_of(delims);
    size_t e = str.find_last_not_of(delims);
    if(b == std::string::npos || e == std::string::npos) {
        return "";
    }
    return std::string(str, b, e - b + 1);
}

std::string allpix::from_string_helper(std::string str) {
    // Check if string is not empty
    str = trim(str);
    if(str.empty()) {
        throw std::invalid_argument("string is empty");
    }

    // Check if there is whitespace in the string
    size_t white_space = str.find_first_of(" \t\n\r\v");
    if(white_space != std::string::npos) {
        throw std::invalid_argument("remaining data at end");
    }
    return str;
}

/**
 * If a pair of enclosing double quotation marks is found, the whole string within the quotation marks is returned.
 * Otherwise only the first part is read until whitespace is encountered.
 */
std::string allpix::from_string_impl(std::string str, type_tag<std::string>) {
    str = trim(str);
    // If there are "" then we should take the whole string
    if(!str.empty() && (str.front() == '\"' || str.front() == '\'')) {
        if(str.find(str.front(), 1) != str.size() - 1) {
            throw std::invalid_argument("remaining data at end");
        }
        return str.substr(1, str.size() - 2);
    }
    // Otherwise read a single string
    return from_string_helper(std::move(str));
}

/**
 * First parse as normal string and then construct path from it.
 */
std::filesystem::path allpix::from_string_impl(std::string str, type_tag<std::filesystem::path>) {
    return {from_string<std::string>(std::move(str))};
}

/**
 * Both numerical (0, 1) and textual representations ("false", "true") are supported for booleans. No enclosing quotation
 * marks should be used.
 */
bool allpix::from_string_impl(std::string str, type_tag<bool>) {
    str = from_string_helper(str);

    std::istringstream sstream(str);
    bool ret_value = false;
    if(isalpha(str.back()) != 0) {
        sstream >> std::boolalpha >> ret_value;
    } else {
        sstream >> ret_value;
    }

    // Check if the reading was successful and everything was read
    if(sstream.fail() || sstream.peek() != EOF) {
        throw std::invalid_argument("conversion not possible");
    }

    return ret_value;
}
