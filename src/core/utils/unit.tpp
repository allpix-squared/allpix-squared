/**
 * @file
 * @brief Template implementation of unit system
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "text.h"

#include <cmath>

namespace allpix {
    /**
     * @throws std::overflow_error If the converted unit overflows the requested type
     * @throws allpix::LogicError If the resulting product of an input integer value and unit is not an integral value
     *
     * The unit type is internally converted to the type \ref Units::UnitType. After multiplying the unit, the output is
     * checked for overflow problems before the type is converted back to the original type.
     */
    template <typename T> T Units::get(T inp, const std::string& str) {
        UnitType out = static_cast<UnitType>(inp) * get(str);
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        if constexpr(std::is_integral_v<T>) {
            // If the input is an integral value: check so that it doesn't have decimals after applying the unit
            if(out != static_cast<T>(out)) {
                throw LogicError("Cannot use integer value with non-integer internal unit; the combination " +
                                 std::to_string(inp) + " " + str + " is invalid.");
            }
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
        if constexpr(std::is_integral_v<T>) {
            out = std::round(out);
        }
        return static_cast<T>(out);
    }
    template <typename T> T Units::getSingleInverse(T inp, std::string str) {
        UnitType out = static_cast<UnitType>(inp) / getSingle(std::move(str));
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        if constexpr(std::is_integral_v<T>) {
            out = std::round(out);
        }
        return static_cast<T>(out);
    }
    template <typename T> T Units::getInverse(T inp, const std::string& str) {
        UnitType out = static_cast<UnitType>(inp) / get(str);
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        if constexpr(std::is_integral_v<T>) {
            out = std::round(out);
        }
        return static_cast<T>(out);
    }

    // Display function for vectors
    template <typename T> std::string Units::display(T inp, std::initializer_list<std::string> units) {
        auto split = allpix::split<Units::UnitType>(allpix::to_string(std::move(inp)));

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
