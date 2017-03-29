/*
 * Support for units
 */

#ifndef ALLPIX_UNIT_H
#define ALLPIX_UNIT_H

#include <limits>
#include <map>
#include <string>
#include <utility>

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
        template <typename T> static T get(std::string str, T inp);
        template <typename T> static T getInverse(std::string str, T inp);

        // convert to another unit from the base unit (FIXME: confusing?)
        static UnitType convert(std::string str, UnitType inp);

    private:
        static std::map<std::string, UnitType> unit_map_;
    };

    // get input in the given unit
    template <typename T> T Units::get(std::string str, T inp) {
        UnitType out = static_cast<UnitType>(inp) * get(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }

    // get input in the inverse of the given unit
    template <typename T> T Units::getInverse(std::string str, T inp) {
        UnitType out = static_cast<UnitType>(inp) / get(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }
}

#endif /* ALLPIX_UNIT_H */
