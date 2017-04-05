#include "unit.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "core/utils/string.h"

using namespace allpix;

std::map<std::string, allpix::Units::UnitType> Units::unit_map_;

// add a new unit
void Units::add(std::string str, allpix::Units::UnitType value) {
    // do not distinguish between different case for units
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    // find the unit
    if(unit_map_.find(str) != unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " already defined");
    }
    unit_map_.emplace(str, value);
}

// get a single unit
allpix::Units::UnitType Units::getSingle(std::string str) {
    if(allpix::trim(str).empty()) {
        throw std::invalid_argument("empty unit is not defined");
    }
    // do not distinguish between different case for units
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    // find the unit
    auto iter = unit_map_.find(str);
    if(iter == unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " not found");
    }
    return iter->second;
}

// get a unit sequence
allpix::Units::UnitType Units::get(std::string str) {
    UnitType ret_value = 1;
    if(allpix::trim(str).empty()) {
        throw std::invalid_argument("empty unit is not defined");
    }

    // go through all units
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
    // apply last unit
    if(lst == '*') {
        ret_value = getSingle(ret_value, unit);
    } else if(lst == '/') {
        ret_value = getSingleInverse(ret_value, unit);
    }
    return ret_value;
}

// convert a unit
// FIXME: maybe support this as a cast...
allpix::Units::UnitType Units::convert(UnitType inp, std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    auto iter = unit_map_.find(str);
    if(iter == unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " not found");
    }

    return (1.0l / iter->second) * inp;
}
