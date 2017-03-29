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
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if(unit_map_.find(str) != unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " already defined");
    }
    unit_map_.emplace(str, value);
}

// get a unit
allpix::Units::UnitType Units::get(std::string str) {
    if(allpix::trim(str).empty()) {
        throw std::invalid_argument("empty unit is not defined");
    }
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    auto iter = unit_map_.find(str);
    if(iter == unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " not found");
    }
    return iter->second;
}

// convert a unit
// FIXME: maybe support this as a cast...
allpix::Units::UnitType Units::convert(std::string str, UnitType inp) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    auto iter = unit_map_.find(str);
    if(iter == unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " not found");
    }

    return (1.0l / iter->second) * inp;
}
