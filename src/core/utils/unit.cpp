/**
 * @file
 * @brief Implementation of unit system
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "unit.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "text.h"

using namespace allpix;

std::map<std::string, allpix::Units::UnitType> Units::unit_map_;

/**
 * @throws std::invalid_argument If the unit already exists
 *
 * Units should consist of only alphabetical characters. Units are converted to lowercase internally. All the defined units
 * should have unique values and it is not possible to redefine them.
 *
 * @note No explicit check is done for alphabetical units
 */
void Units::add(std::string str, UnitType value) {
    // Do not distinguish between different case for units
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    // Find the unit
    if(unit_map_.find(str) != unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " already defined");
    }
    unit_map_.emplace(str, value);
}

/**
 * @throws std::invalid_argument If the requested unit does not exist
 *
 * All units are converted to lowercase before finding their value in the internal database.
 */
allpix::Units::UnitType Units::getSingle(std::string str) {
    if(allpix::trim(str).empty()) {
        // An empty unit equals a multiplication with one:
        return 1.;
    }
    // Do not distinguish between different case for units
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    // Find the unit
    auto iter = unit_map_.find(str);
    if(iter == unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " not found");
    }
    return iter->second;
}

/**
 * @throws invalid_argument If one tries to get the value of an empty unit
 *
 * Units are combined by applying the multiplication operator * and the division operator / linearly. The first unit is
 * always multiplied, following common sense. Grouping units together within brackets or parentheses is not supported. Thus
 * any other character then a name of a unit, * or \ should lead to an error
 */
allpix::Units::UnitType Units::get(const std::string& str) {
    UnitType ret_value = 1;
    if(allpix::trim(str).empty()) {
        return ret_value;
    }

    // Go through all units
    char lst = '*';
    std::string unit;
    for(char ch : str) {
        if(ch == '*' || ch == '/') {
            if(lst == '*') {
                ret_value = getSingle(ret_value, unit);
                unit.clear();
            } else if(lst == '/') {
                ret_value = getSingleInverse(ret_value, unit);
                unit.clear();
            }
            lst = ch;
        } else {
            unit += ch;
        }
    }

    // Apply last unit
    if(lst == '*') {
        ret_value = getSingle(ret_value, unit);
    } else if(lst == '/') {
        ret_value = getSingleInverse(ret_value, unit);
    }
    return ret_value;
}

allpix::Units::UnitType Units::convert(UnitType input, std::string str) {
    // Do not distinguish between different case for units
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    // Go through all units
    char lst = '*';
    std::string unit;
    for(char ch : str) {
        if(ch == '*' || ch == '/') {
            if(lst == '*') {
                input = getSingleInverse(input, unit);
                unit.clear();
            } else if(lst == '/') {
                input = getSingle(input, unit);
                unit.clear();
            }
            lst = ch;
        } else {
            unit += ch;
        }
    }
    // Handle last unit
    if(lst == '*') {
        input = getSingleInverse(input, unit);
    } else if(lst == '/') {
        input = getSingle(input, unit);
    }
    return input;
}

/**
 * @throws std::invalid_argument If the list of units is empty
 *
 * The best unit is determined using the rules below:
 * - If the input is zero, the best unit can't be determined and the first one will be used.
 * - If there exists at least one unit for which the value is larger than one, the unit with value nearest to one is chosen
 *   from all units with values larger than one
 * - Otherwise the unit is chosen that has a value as close as possible to one (from below)
 */
std::string Units::display(UnitType input, std::initializer_list<std::string> units) {
    if(units.size() == 0) {
        throw std::invalid_argument("list of possible units cannot be empty");
    }

    std::ostringstream stream;
    // Find best unit
    int best_exponent = std::numeric_limits<int>::min();
    std::string best_unit;
    for(const auto& unit : units) {
        Units::UnitType value = convert(input, unit);
        int exponent = 0;
        std::frexp(value, &exponent);
        if((best_exponent <= 0 && exponent > best_exponent) || (exponent > 0 && exponent < best_exponent)) {
            best_exponent = exponent;
            best_unit = unit;
        }
    }

    // Write unit
    stream << convert(input, best_unit);
    stream << best_unit;
    return stream.str();
}
std::string Units::display(UnitType input, std::string unit) { return display(input, {std::move(unit)}); }
