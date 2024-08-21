/**
 * @file
 * @brief Template implementation of unit system
 *
 * @copyright Copyright (c) 2018-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "text.h"

#include <cmath>
#include <iostream>

namespace allpix {
    /**
     * @throws std::overflow_error If the converted unit overflows the requested type
     *
     * The unit type is internally converted to the type \ref Units::UnitType. After multiplying the unit, the output is
     * checked for overflow problems before the type is converted back to the original type.
     */
    //std::enable_if<std::is_integral_v<T>>
    template <typename T, std::enable_if_t<!std::is_integral_v<T>, bool> = true> T Units::get(T inp, const std::string& str) {
        std::cout << "Input value: " << inp << std::endl;
        UnitType out = static_cast<UnitType>(inp) * get(str);
        std::cout << "Middle value: " << out << std::endl;
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        std::cout << "Output value 1: " << out << std::endl;
        std::cout << "Output value final: " << static_cast<T>(out) << std::endl;
        return static_cast<T>(out);
        //return out;
    }
    
    template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true> T Units::get(T inp, const std::string& str) {
        std::cout << "Input value int: " << inp << std::endl;
        UnitType out = static_cast<UnitType>(inp) * get(str);
        std::cout << "Middle value int: " << out << std::endl;
        if(out > static_cast<UnitType>(std::numeric_limits<T>::max()) ||
           out < static_cast<UnitType>(std::numeric_limits<T>::lowest())) {
            throw std::overflow_error("unit conversion overflows the type");
        }
        std::cout << "--------------- Output value assert: " << out << ", " << static_cast<T>(out) << std::endl;
        if(out != static_cast<T>(out)) {
            throw std::invalid_argument("Cannot use integer value with non-integer internal unit; the combination " + std::to_string(inp) + " " + str + " is invalid.");
        }
        assert(out == static_cast<T>(out));
        //std::cout << "--------------- Output value assert: " << out << ", " << static_cast<T>(out) << std::endl;
        std::cout << "Output value 1 int: " << out << std::endl;
        std::cout << "Output value final int: " << static_cast<T>(out) << std::endl;
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
