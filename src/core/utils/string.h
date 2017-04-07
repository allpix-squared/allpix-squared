/**
 * From and to string conversions (mostly necessary for config parsing)
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

namespace allpix {

    /** Trims the leading and trailing white space from a string
     */
    inline std::string trim(const std::string& s, const std::string& delims = " \t\n\r\v") {
        size_t b = s.find_first_not_of(delims);
        size_t e = s.find_last_not_of(delims);
        if(b == std::string::npos || e == std::string::npos) {
            return "";
        }
        return std::string(s, b, e - b + 1);
    }

    /** Converts a string to any type
     */
    template <typename T> T from_string(std::string str) {
        // use tag dispatch to select the correct implementation
        return from_string_impl(str, type_tag<T>());
    }

    // FIXME: include exceptions better
    // helper functions to do cleaning and checks for string reading
    static std::string _from_string_helper(std::string str) {
        str = trim(str);
        if(str == "") {
            throw std::invalid_argument("string is empty");
        }

        size_t white_space = str.find_first_of(" \t\n\r\v");
        if(white_space != std::string::npos) {
            throw std::invalid_argument("remaining data at end");
        }
        return str;
    }

    // fetch for all types that have not a supported conversion
    template <typename T, typename = std::enable_if_t<!std::is_arithmetic<T>::value>, typename = void>
    T from_string_impl(const std::string&, type_tag<T>) {
        // check if correct conversion
        static_assert(std::is_same<T, void>::value,
                      "Conversion to this type is not implemented: an overload should be added to support this conversion");
        return T();
    }
    // overload for arithmetic types
    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    T from_string_impl(std::string str, type_tag<T>) {
        str = _from_string_helper(str);

        // find an optional unit
        auto unit_idx = str.size() - 1;
        for(; unit_idx > 0; --unit_idx) {
            if(!isalpha(str[unit_idx]) && str[unit_idx] != '*' && str[unit_idx] != '/') {
                break;
            }
        }
        std::string units = str.substr(unit_idx + 1);

        // get the actual value
        std::istringstream sstream(str.substr(0, unit_idx + 1));
        T ret_value = 0;
        sstream >> ret_value;
        if(sstream.fail() || !sstream.eof()) {
            throw std::invalid_argument("conversion not possible");
        }

        // apply all the units if they exists
        if(!units.empty()) {
            ret_value = allpix::Units::get(ret_value, units);
        }
        return ret_value;
    }
    // overload for string
    inline std::string from_string_impl(std::string str, type_tag<std::string>) {
        str = trim(str);
        // if there are "" then we should take the whole string (FIXME: '' should also be supported)
        if(!str.empty() && str[0] == '\"') {
            if(str.find('\"', 1) != str.size() - 1) {
                throw std::invalid_argument("remaining data at end");
            }
            return str.substr(1, str.size() - 2);
        }
        // otherwise read a single string
        return _from_string_helper(str);
    }

    /** Converts supported type to a string
     */
    template <typename T> std::string to_string(T inp) {
        // use tag dispatch to select the correct implementation
        return to_string_impl(inp, empty_tag());
    }

    // catch all overload for not implemented conversion
    template <typename T, typename = std::enable_if_t<!std::is_arithmetic<T>::value>, typename = void>
    void to_string_impl(T, empty_tag) {
        static_assert(std::is_same<T, void>::value,
                      "Conversion to this type is not implemented: an overload should be added to support this conversion");
    }
    // overload for arithmetic types
    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    std::string to_string_impl(T inp, empty_tag) {
        std::ostringstream out;
        out << inp;
        return out.str();
    }
    // NOTE: we have to provide all these specializations to prevent std::to_string from being called
    //       there may be a better way to work with this
    inline std::string to_string_impl(const std::string& inp, empty_tag) { return '"' + inp + '"'; }
    inline std::string to_string_impl(const char* inp, empty_tag) { return '"' + std::string(inp) + '"'; }
    inline std::string to_string_impl(char* inp, empty_tag) { return '"' + std::string(inp) + '"'; }

    /** Splits string s into elements at delimiter "delim" and returns them as vector
     */
    template <typename T> std::vector<T> split(std::string str, std::string delims = " ,") {
        str = trim(str, delims);

        // if the input string is empty, simply return empty container
        if(str.empty()) {
            return std::vector<T>();
        }

        // else we have data, clear the default elements and chop the string:
        std::vector<T> elems;

        // add the string identifiers as special delims
        delims += "\'\"";

        std::size_t prev = 0, sprev = 0, pos;
        char ins = 0;
        while((pos = str.find_first_of(delims, sprev)) != std::string::npos) {
            sprev = pos + 1;

            // FIXME: handle escape
            if(str[pos] == '\'' || str[pos] == '\"') {
                if(!ins) {
                    ins = str[pos];
                } else if(ins == str[pos]) {
                    ins = 0;
                }
                continue;
            }
            if(ins) {
                continue;
            }

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
