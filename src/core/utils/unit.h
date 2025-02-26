/**
 * @file
 * @brief System to support units in the framework
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_UNIT_H
#define ALLPIX_UNIT_H

#include <initializer_list>
#include <limits>
#include <map>
#include <string>
#include <utility>

// TODO [doc] Check if this class can be constexpressed?

namespace allpix {

    /**
     * @brief Static class to access units
     * @see The list of framework units defined in \ref allpix::register_units
     *
     * Units are short, unique and case-insensitive strings that indicate a particular multiplication factor from the base
     * unit in the framework. The unit system can convert external types to the system units and vise-versa for displaying
     * purposes. Inside the framework only the defaults unit should be used, either directly or through a direct conversion.
     */
    class Units {
    public:
        /**
         * @brief Type used to store the units
         */
        using UnitType = long double;

        /**
         * @brief Delete default constructor (only static access)
         */
        Units() = delete;

        /**
         * @brief Add a new unit to the system
         * @param str Identifier of the unit
         * @param value Multiplication factor from the base unit
         * @throws std::invalid_argument if unit is already defined
         */
        static void add(std::string str, UnitType value);

        /**
         * @brief Get value of a single unit in the base units
         * @param str Name of the unit
         * @return Value in the base unit
         */
        // TODO [doc] This function should likely be removed
        static UnitType getSingle(std::string str);
        /**
         * @brief Get single input parameter in the base units
         * @param inp Value in a particular unit
         * @param str Name of that particular unit
         * @return Value in the base unit
         */
        // TODO [doc] This function should likely be removed
        template <typename T> static T getSingle(T inp, std::string str);
        /**
         * @brief Get single input parameter in the inverse of the base units
         * @param inp Value in a particular unit
         * @param str Name of that particular unit
         * @return Value in the base unit
         */
        // TODO [doc] This function should likely be removed
        template <typename T> static T getSingleInverse(T inp, std::string str);

        /**
         * @brief Get value of a unit in the base units
         * @param str Name of the unit
         * @return Value in the base unit
         * @warning Conversions should not be done with the result of this function. The \ref Units::get(T, const
         * std::string&) version should be used for that purpose instead.
         */
        static UnitType get(const std::string& str);
        /**
         * @brief Get input parameter in the base units
         * @param inp Value in a particular unit
         * @param str Name of that particular unit
         * @return Value in the base unit
         */
        template <typename T> static T get(T inp, const std::string& str);
        /**
         * @brief Get input parameter in the inverse of the base units
         * @param inp Value in a particular unit
         * @param str Name of that particular unit
         * @return Value in the base unit
         */
        // TODO [doc] This function should likely be removed
        template <typename T> static T getInverse(T inp, const std::string& str);

        /**
         * @brief Get base unit in the requested unit
         * @param input Value in the base unit system
         * @param str Name of the output unit
         * @return Value in the requested unit
         */
        // TODO [doc] This function should maybe be removed
        // TODO [doc] Shall we change the name in something better here
        static UnitType convert(UnitType input, std::string str);

        /**
         * @brief Return value for display in the best of all the provided units
         * @param input Value in the base unit system
         * @param units Name of the possible output units
         * @return Value with best unit to be used for display
         */
        static std::string display(UnitType input, std::initializer_list<std::string> units);
        /**
         * @brief Return value in the requested unit for display
         * @param input Value in other type
         * @param unit Name of the output unit
         * @return Value with unit to be used for display
         */
        static std::string display(UnitType input, std::string unit);
        /**
         * @brief Return value for display in the best of all the provided units for all vector elements
         * @param input Vector of values in the base unit system
         * @param units Name of the possible output units
         * @return Vector of values between parentheses with best display units for each element
         * @note Works for all vector types that can be converted to a string using \ref StringConversions "the string
         * utilities".
         */
        template <typename T> static std::string display(T input, std::initializer_list<std::string> units);

    private:
        static std::map<std::string, UnitType> unit_map_;
    };
} // namespace allpix

// Include template definitions
#include "unit.tpp"

#endif /* ALLPIX_UNIT_H */
