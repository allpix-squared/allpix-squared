/*
 * Support for units
 */

#ifndef ALLPIX_UNIT_H
#define ALLPIX_UNIT_H

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace allpix {

    // FIXME: a lowercase namespace instead?
    class Units {
    public:
        using UnitType = long double;

        // allow only static access : delete constructor
        Units() = delete;

        // add a new unit
        static void add(std::string str, UnitType value);

        // get a new unit
        static UnitType get(std::string str);

        // convert to another unit
        static UnitType convert(std::string str, UnitType inp);

    private:
        static std::map<std::string, UnitType> unit_map_;
    };
}

#endif /* ALLPIX_UNIT_H */
