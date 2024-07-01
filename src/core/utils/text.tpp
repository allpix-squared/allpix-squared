/**
 * @file
 * @brief Template implementation of string utilities
 *
 * @copyright Copyright (c) 2018-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Used extensively for parsing the configuration in the \ref allpix::ConfigReader.
 */

#include <algorithm>
#include <numeric>

#include <magic_enum/magic_enum.hpp>

#include "unit.h"

namespace allpix {
    /**
     * The matching converter function is automatically found if available. To add a new conversion the \ref from_string_impl
     * function should be overloaded. The string is passed as first argument to this function, the second argument should be
     * an \ref allpix::type_tag with the type to convert to.
     */
    template <typename T> T from_string(std::string str) {
        // Use tag dispatch to select the correct helper function
        return from_string_impl(std::move(str), type_tag<T>());
    }

    /**
     * The Magic Enum library is used for conversion between string and enum type
     */
    template <typename T>
    typename std::enable_if_t<std::is_enum<T>::value, T> from_string_impl(std::string str, type_tag<T>) {
        str = from_string_impl(str, type_tag<std::string>());

        auto val = magic_enum::enum_cast<T>(str, magic_enum::case_insensitive);
        if(val.has_value()) {
            return val.value();
        }

        // Generate list of available values for the exception:
        auto v = magic_enum::enum_names<T>();
        std::string vstr = std::accumulate(v.begin(), v.end(), std::string(), [](auto a, auto s) {
            return a + (a.empty() ? "" : ", ") + std::string(s.data());
        });
        std::transform(vstr.begin(), vstr.end(), vstr.begin(), ::tolower);
        throw std::invalid_argument("invalid value, possible values are: " + vstr);
    }

    /**
     * The unit system is used through \ref Units::get to parse unit suffixes and convert the values to the appropriate
     * standard framework unit.
     */
    template <typename T>
    typename std::enable_if_t<std::is_arithmetic<T>::value, T> from_string_impl(std::string str, type_tag<T>) {
        str = from_string_helper(str);

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

        // Check if the reading was successful and everything was read
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
     * The matching converter function is automatically found if available. To add a new conversion the \ref to_string_impl
     * function should be overloaded. The string is passed as first argument to this function, the second argument should be
     * an \ref allpix::empty_tag (required to search in the allpix namespace).
     */
    template <typename T> std::string to_string(T inp) {
        // Use tag dispatch to select the correct implementation
        return to_string_impl(inp, empty_tag());
    }

    template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool>>
    std::string to_string_impl(T inp, empty_tag) {
        std::ostringstream out;
        out << inp;
        return out.str();
    }

    template <typename T, std::enable_if_t<std::is_enum<T>::value, bool>> std::string to_string_impl(T inp, empty_tag) {
        return allpix::transform(std::string(magic_enum::enum_name(inp)), ::tolower);
    }

    template <typename T> std::vector<T> split(std::string str, const std::string& delims) {
        str = trim(str, delims);

        // If the input string is empty, simply return empty container
        if(str.empty()) {
            return std::vector<T>();
        }

        // Else we have data, clear the default elements and chop the string:
        std::vector<T> elems;

        // Loop through the string
        std::size_t prev = 0, pos = 0;
        while((pos = str.find_first_of(delims, prev)) != std::string::npos) {
            if(pos > prev) {
                elems.push_back(from_string<T>(str.substr(prev, pos - prev)));
            }
            prev = pos + 1;
        }
        if(prev < str.length()) {
            elems.push_back(from_string<T>(str.substr(prev, std::string::npos))); // NOLINT
        }

        return elems;
    }

    template <typename T> std::string transform(const std::string& str, const T& op) {
        auto output = str;
        std::transform(output.begin(), output.end(), output.begin(), op);
        return output;
    }
} // namespace allpix
