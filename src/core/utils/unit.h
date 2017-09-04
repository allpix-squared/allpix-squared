/**
 * @file
 * @brief System to support units in the framework
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
     * @see The list of framework units defined in \ref Allpix::add_units
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
         * @warning Conversions should not be done with the result of this function. The \ref get(std::string) version should
         *          be used for that purpose instead.
         */
        static UnitType get(std::string str);
        /**
         * @brief Get input parameter in the base units
         * @param inp Value in a particular unit
         * @param str Name of that particular unit
         * @return Value in the base unit
         */
        template <typename T> static T get(T inp, std::string str);
        /**
         * @brief Get input parameter in the inverse of the base units
         * @param inp Value in a particular unit
         * @param str Name of that particular unit
         * @return Value in the base unit
         */
        // TODO [doc] This function should likely be removed
        template <typename T> static T getInverse(T inp, std::string str);

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
        // TODO [doc] Shall we change the name in something better here
        static std::string display(UnitType input, std::initializer_list<std::string> units);
        /**
         * @brief Return value in the requested unit for display
         * @param input Value in other type
         * @param unit Name of the output unit
         * @return Value with unit to be used for display
         */
        // TODO [doc] Shall we change the name in something better here
        static std::string display(UnitType input, std::string unit);

    private:
        static std::map<std::string, UnitType> unit_map_;
    };

    // TODO [doc] Move these to a separate template implementation file?

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
} // namespace allpix

#endif /* ALLPIX_UNIT_H */
