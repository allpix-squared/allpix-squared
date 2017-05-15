/**
 * @file
 * @brief Implementation of unit system
 *
 * @copyright MIT License
 */

#include "unit.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "core/utils/string.h"

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
void Units::add(std::string str, allpix::Units::UnitType value) {
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
        throw std::invalid_argument("empty unit is not defined");
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
allpix::Units::UnitType Units::get(std::string str) {
    UnitType ret_value = 1;
    if(allpix::trim(str).empty()) {
        throw std::invalid_argument("empty unit is not defined");
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

// Convert the base unit to another one
allpix::Units::UnitType Units::convert(UnitType inp, std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    auto iter = unit_map_.find(str);
    if(iter == unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " not found");
    }

    return (1.0l / iter->second) * inp;
}
