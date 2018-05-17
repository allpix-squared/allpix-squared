#include "text.h"

namespace allpix {
    /**
     * @throws std::overflow_error If the converted unit overflows the requested type
     *
     * The unit type is internally converted to the type \ref Units::UnitType. After multiplying the unit, the output is
     * checked for overflow problems before the type is converted back to the original type.
     */
    template <typename T> T Units::get(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) * get(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }

    // Getters for single and inverse units
    template <typename T> T Units::getSingle(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) * getSingle(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }
    template <typename T> T Units::getSingleInverse(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) / getSingle(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }
    template <typename T> T Units::getInverse(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) / get(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        return static_cast<T>(out);
    }

    // Display function for vectors
    template <typename T> std::string Units::display(T inp, std::initializer_list<std::string> units) {
        auto split = allpix::split<Units::UnitType>(allpix::to_string(inp));

        std::string ret_str;
        if(split.size() > 1) {
            ret_str += "(";
        }

        for(auto& element : split) {
            ret_str += Units::display(element, units);
            ret_str += ",";
        }

        if(split.size() > 1) {
            ret_str[ret_str.size() - 1] = ')';
        } else {
            ret_str.pop_back();
        }

        return ret_str;
    }
} // namespace allpix
