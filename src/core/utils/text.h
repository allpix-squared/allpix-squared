/**
 * @file
 * @brief Collection of string utilities
 *
 * @copyright Copyright (c) 2016-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Used extensively for parsing the configuration in the \ref allpix::ConfigReader.
 */

/**
 * @defgroup StringConversions String conversions
 * @brief Collection of all the overloads of string conversions
 */

#ifndef ALLPIX_TEXT_H
#define ALLPIX_TEXT_H

#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "type.h"

namespace allpix {

    /**
     * @brief Trims leading and trailing characters from a string
     * @param str String that should be trimmed
     * @param delims List of delimiters to trim from the string (defaults to all whitespace)
     */
    std::string trim(const std::string& str, const std::string& delims = " \t\n\r\v");

    /**
     * @brief Converts a string to any supported type
     * @param str String to convert
     * @see StringConversions
     */
    template <typename T> T from_string(std::string str);

    /**
     * @brief Internal helper method for checking and trimming conversions from string
     * @param str Input string to check and trim
     * @return Trimmed string
     */
    std::string from_string_helper(std::string str);

    /**
     * @ingroup StringConversions
     * @brief Conversion handler for all arithmetic types
     * @throws std::invalid_argument If the string cannot be converted to the required arithmetic type
     */
    template <typename T>
    typename std::enable_if_t<std::is_arithmetic_v<T>, T> from_string_impl(std::string str, type_tag<T> /*unused*/);
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for all enum types
     * @throws std::invalid_argument If the string cannot be converted to the required enum type
     */
    template <typename T>
    typename std::enable_if_t<std::is_enum_v<T>, T> from_string_impl(std::string str, type_tag<T> /*unused*/);
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for strings
     * @throws std::invalid_argument If no closing quotation mark as last character after an opening quotation mark
     * @throws std::invalid_argument If string without enclosing quotation marks, but more data after whitespace is found
     */
    std::string from_string_impl(std::string str, type_tag<std::string> /*unused*/);
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for filesystem paths
     * @throws std::invalid_argument If no closing quotation mark as last character after an opening quotation mark
     * @throws std::invalid_argument If string without enclosing quotation marks, but more data after whitespace is found
     */
    std::filesystem::path from_string_impl(std::string str, type_tag<std::filesystem::path> /*unused*/);

    /**
     * @ingroup StringConversions
     * @brief Conversion handler for booleans
     * @throws std::invalid_argument If the string cannot be converted to a boolean type
     */
    bool from_string_impl(std::string str, type_tag<bool> /*unused*/);

    /**
     * @brief Converts any type to a string
     * @note C-strings are not supported due to allocation issues
     */
    template <typename T> std::string to_string(const T& inp);

    /**
     * @ingroup StringConversions
     * @brief Conversion handler for all arithmetic types
     */
    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    std::string to_string_impl(T inp, empty_tag /*unused*/);
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for all enum types
     */
    template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    std::string to_string_impl(T inp, empty_tag /*unused*/);

    /// @{
    /**
     * @ingroup StringConversions
     * @brief Conversion handler for strings
     * @note Overloaded for different types of strings
     *
     * Adds enclosing double quotation marks to properly store strings containing whitespace.
     */
    inline std::string to_string_impl(const std::string& inp, empty_tag /*unused*/) { return '"' + inp + '"'; }
    inline std::string to_string_impl(const char* inp, empty_tag /*unused*/) { return '"' + std::string(inp) + '"'; }
    inline std::string to_string_impl(char* inp, empty_tag /*unused*/) { return '"' + std::string(inp) + '"'; }
    /// @}

    /**
     * @brief Splits string into substrings at delimiters
     * @param str String to split
     * @param delims Delimiters to split at (defaults to space, tab and comma)
     * @return List of all the substrings with all empty substrings ignored (thus removed)
     */
    template <typename T> std::vector<T> split(std::string str, const std::string& delims = " \t,");

    /**
     * @brief Transforms a string and returns the transformed string
     * @param str String that should be transformed
     * @param op Unary operator to act on the string
     * @return Transformed string
     */
    template <typename T> std::string transform(const std::string& str, const T& op);

} // namespace allpix

// Include template definitions
#include "text.tpp"

#endif /* ALLPIX_TEXT_H */
