/**
 * @file
 * @brief Collection of string utilities
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * Used extensively for parsing the configuration in the \ref allpix::ConfigReader.
 */

/**
 * @defgroup StringConversions String conversions
 * @brief Collection of all the overloads of string conversions
 */

#ifndef ALLPIX_STRING_H
#define ALLPIX_STRING_H

#include <cctype>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "type.h"
#include "unit.h"

// TODO [doc]: should possible be put in a separate namespace

namespace allpix {

    /**
     * @brief Trims leading and trailing characters from a string
     * @param str String that should be trimmed
     * @param delims List of delimiters to trim from the string (defaults to all whitespace)
     */
    inline std::string trim(const std::string& str, const std::string& delims = " \t\n\r\v") {
        size_t b = str.find_first_not_of(delims);
        size_t e = str.find_last_not_of(delims);
        if(b == std::string::npos || e == std::string::npos) {
            return "";
        }
        return std::string(str, b, e - b + 1);
    }

    /**
     * @brief Converts a string to any supported type
     * @param str String to convert
     * @see StringConversions
     *
     * The matching converter function is automatically found if available. To add a new conversion the \ref from_string_impl
     * function should be overloaded. The string is passed as first argument to this function, the second argument should be
     * an \ref allpix::type_tag with the type to convert to.
     *
     */
    template <typename T> T from_string(std::string str) {
        // Use tag dispatch to select the correct helper function
        return from_string_impl(str, type_tag<T>());
    }

    // TODO [doc] This should move to a source file
    // FIXME: include exceptions better
    // helper functions to do cleaning and checks for string reading
    static std::string _from_string_helper(std::string str) {
        // Check if string is not empty
        str = trim(str);
        if(str.empty()) {
            throw std::invalid_argument("string is empty");
        }

        // Check if there is whitespace in the string
        size_t white_space = str.find_first_of(" \t\n\r\v");
        if(white_space != std::string::npos) {
            throw std::invalid_argument("remaining data at end");
        }
        return str;
    }

    /**
     * @ingroup StringConversions
     * @brief Conversion handler for all non implemented conversions
     *
     * Function does not return but will raise an static assertion.
     */
    template <typename T, typename = std::enable_if_t<!std::is_arithmetic<T>::value>, typename = void>
    constexpr T from_string_impl(const std::string&, type_tag<T>) {
        static_assert(std::is_same<T, void>::value,
                      "Conversion to this type is not implemented: an overload should be added to support this conversion");
        return T();
    }
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for all arithmetic types
     * @throws std::invalid_argument If the string cannot be converted to the required arithmetic type
     *
     * The unit system is used through \ref Units::get to parse unit suffixes and convert the values to the appropriate
     * standard framework unit.
     */
    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    T from_string_impl(std::string str, type_tag<T>) {
        str = _from_string_helper(str);

        // Find an optional set of units
        auto unit_idx = str.size() - 1;
        for(; unit_idx > 0; --unit_idx) {
            if(!isalpha(str[unit_idx]) && str[unit_idx] != '*' && str[unit_idx] != '/') {
                break;
            }
        }
        std::string units = str.substr(unit_idx + 1);

        // Get the actual arithmetic value
        std::istringstream sstream(str.substr(0, unit_idx + 1));
        T ret_value = 0;
        sstream >> ret_value;

        // Check if the reading was succesfull and everything was read
        if(sstream.fail() || sstream.peek() != EOF) {
            throw std::invalid_argument("conversion not possible");
        }

        // Apply all the units if they exists
        if(!units.empty()) {
            ret_value = allpix::Units::get(ret_value, units);
        }
        return ret_value;
    }
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for strings
     * @throws std::invalid_argument If the string has no closing quotation mark as last character after an opening quotation
     * mark
     * @throws std::invalid_argument If the string has no enclosing quotation marks but contains more data after whitespace
     * is found
     *
     * If a pair of enclosing double quotation marks is found, the whole string within the quotation marks is returned.
     * Otherwise only the first part is read until whitespace is encountered.
     */
    inline std::string from_string_impl(std::string str, type_tag<std::string>) {
        str = trim(str);
        // If there are "" then we should take the whole string
        if(!str.empty() && (str.front() == '\"' || str.front() == '\'')) {
            if(str.find(str.front(), 1) != str.size() - 1) {
                throw std::invalid_argument("remaining data at end");
            }
            return str.substr(1, str.size() - 2);
        }
        // Otherwise read a single string
        return _from_string_helper(str);
    }

    /**
     * @ingroup StringConversions
     * @brief Conversion handler for booleans
     * @throws std::invalid_argument If the string cannot be converted to a boolean type
     *
     * Converts both numerical (0, 1) and textual representations ("false", "true") are supported. No enclosing quotation
     * marks should be used.
     */
    inline bool from_string_impl(std::string str, type_tag<bool>) {
        str = _from_string_helper(str);

        std::istringstream sstream(str);
        bool ret_value = false;
        if(isalpha(str.back()) != 0) {
            sstream >> std::boolalpha >> ret_value;
        } else {
            sstream >> ret_value;
        }

        // Check if the reading was succesfull and everything was read
        if(sstream.fail() || sstream.peek() != EOF) {
            throw std::invalid_argument("conversion not possible");
        }

        return ret_value;
    }

    /**
     * @brief Converts any type to a string
     * @note C-strings are not supported due to allocation issues
     *
     * The matching converter function is automatically found if available. To add a new conversion the \ref to_string_impl
     * function should be overloaded. The string is passed as first argument to this function, the second argument should be
     * an \ref allpix::empty_tag (needed to search in the allpix namespace).
     */
    template <typename T> std::string to_string(T inp) {
        // Use tag dispatch to select the correct implementation
        return to_string_impl(inp, empty_tag());
    }

    /**
     * @ingroup StringConversions
     * @brief Conversion handler for all non implemented conversions
     *
     * Function does not return but will raise an static assertion.
     */
    template <typename T, typename = std::enable_if_t<!std::is_arithmetic<T>::value>, typename = void>
    constexpr void to_string_impl(T, empty_tag) {
        static_assert(std::is_same<T, void>::value,
                      "Conversion to this type is not implemented: an overload should be added to support this conversion");
    }
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for all arithmetic types
     */
    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    std::string to_string_impl(T inp, empty_tag) {
        std::ostringstream out;
        out << inp;
        return out.str();
    }

    ///@{
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for strings
     * @note Overloaded for different types of strings
     *
     * Adds enclosing double quotation marks to properly store strings containing whitespace.
     */
    inline std::string to_string_impl(const std::string& inp, empty_tag) { return '"' + inp + '"'; }
    inline std::string to_string_impl(const char* inp, empty_tag) { return '"' + std::string(inp) + '"'; }
    inline std::string to_string_impl(char* inp, empty_tag) { return '"' + std::string(inp) + '"'; }
    ///@}

    /**
     * @brief Splits string into substrings at delimiters
     * @param str String to split
     * @param delims Delimiters to split at
     * @return List of all the substrings with all empty substrings ignored (thus removed)
     */
    template <typename T> std::vector<T> split(std::string str, const std::string& delims = " \t,") {
        str = trim(str, delims);

        // If the input string is empty, simply return empty container
        if(str.empty()) {
            return std::vector<T>();
        }

        // Else we have data, clear the default elements and chop the string:
        std::vector<T> elems;

        // Loop through the string
        std::size_t prev = 0, pos;
        while((pos = str.find_first_of(delims, prev)) != std::string::npos) {
            if(pos > prev) {
                elems.push_back(from_string<T>(str.substr(prev, pos - prev)));
            }
            prev = pos + 1;
        }
        if(prev < str.length()) {
            elems.push_back(from_string<T>(str.substr(prev, std::string::npos)));
        }

        return elems;
    }
} // namespace allpix

#endif /* ALLPIX_STRING_H */
