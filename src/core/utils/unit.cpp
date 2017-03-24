#include "unit.h"

#include <algorithm>
#include <string>

using namespace allpix;

std::map<std::string, allpix::Units::UnitType> Units::unit_map_;

void Units::add(std::string str, allpix::Units::UnitType value) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if(unit_map_.find(str) != unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " already defined");
    }
    unit_map_.emplace(str, value);
}

// get a new unit
allpix::Units::UnitType Units::get(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    auto iter = unit_map_.find(str);
    if(iter == unit_map_.end()) {
        throw std::invalid_argument("unit " + str + " not found");
    }
    return iter->second;
}
