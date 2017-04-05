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

        // get a single unit
        static UnitType getSingle(std::string str);
        template <typename T> static T getSingle(T inp, std::string str);
        template <typename T> static T getSingleInverse(T inp, std::string str);

        // get a unit sequence
        static UnitType get(std::string str);
        template <typename T> static T get(T inp, std::string str);
        template <typename T> static T getInverse(T inp, std::string str);

        // convert to another unit from the base unit (FIXME: confusing?)
        static UnitType convert(UnitType inp, std::string str);

    private:
        static std::map<std::string, UnitType> unit_map_;
    };

    // get single input in the given unit
    template <typename T> T Units::getSingle(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) * getSingle(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }

    // get single input in the inverse of the given unit
    template <typename T> T Units::getSingleInverse(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) / getSingle(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }

    // get input in the given unit
    template <typename T> T Units::get(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) * get(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }

    // get input in the inverse of the given unit
    template <typename T> T Units::getInverse(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) / get(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }

} // namespace allpix

#endif /* ALLPIX_UNIT_H */
