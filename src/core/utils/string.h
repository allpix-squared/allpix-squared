/**
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 */

#ifndef ALLPIX_STRING_H
#define ALLPIX_STRING_H

#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace allpix {

    /** Trims the leading and trailing white space from a string
     */
    inline std::string trim(const std::string &s, const std::string &delims = " \t\n\r\v") {
        size_t b = s.find_first_not_of(delims);
        size_t e = s.find_last_not_of(delims);
        if (b == std::string::npos || e == std::string::npos) {
            return "";
        }
        return std::string(s, b, e - b + 1);
    }

    /** Converts a string to any type.
     * \param x The string to be converted.
     * \param def The default value to be used in case of an invalid string,
     *            this can also be useful to select the correct template type
     *            without having to specify it explicitly.
     * \return An object of type T with the value represented in x, or if
     *         that is not valid then the value of def.
     */

    // FIXME: include exceptions better
    template <typename T> T from_string(std::string str) {
        str = trim(str);
        if (str == "") {
          throw std::invalid_argument("string is empty");
        }

        T ret;
        std::istringstream stream(str);
        stream >> ret;

        std::string tmp;
        stream >> tmp;
        if (!tmp.empty()) {
          throw std::invalid_argument("remaining data at end");
        }
        return ret;
    }
    template <> inline std::string from_string<std::string> (std::string str){
      if (!str.empty() && str[0] == '\"') {
        str.erase(str.begin());
      }
      if (!str.empty() && str[str.size() - 1] == '\"') {
        str.erase(--str.end());
      }
      return str;
    }

    template <typename T> std::string to_string(T inp) {
        return std::to_string(inp);
    }
    // NOTE: we have to provide all these specializations to prevent std::to_string from being called
    //       there may be a better way to work with this
    inline std::string to_string(std::string inp) {
        return inp;
    }
    inline std::string to_string(const char inp[]) {
        return inp;
    }
    inline std::string to_string(char inp[]) {
        return inp;
    }

    /** Splits string s into elements at delimiter "delim" and returns them as vector
     */
    template <typename T> std::vector<T> split(std::string str, std::string delims = " ,") {
        str = trim(str, delims);

        // if the input string is empty, simply return empty container
        if (str.empty()) {
          return std::vector<T>();
        }

        // else we have data, clear the default elements and chop the string:
        std::vector<T> elems;

        //add the string identifiers as special delims
        delims += "\'\"";

        std::size_t prev = 0, sprev = 0, pos;
        char ins = 0;
        while ((pos = str.find_first_of(delims, sprev)) != std::string::npos) {
            sprev = pos+1;

            // FIXME: handle escape
            if (str[pos] == '\'' || str[pos] == '\"') {
              if (!ins) {
                ins = str[pos];
              } else if (ins == str[pos]) {
                ins = 0;
              }
              continue;
            }
            if (ins) {
              continue;
            }

            if (pos > prev) {
              elems.push_back(from_string<T>(str.substr(prev, pos - prev)));
            }
            prev = pos+1;
        }
        if (prev < str.length()) {
          elems.push_back(from_string<T>(str.substr(prev, std::string::npos)));
        }

        return elems;
    }

}

#endif /* ALLPIX_STRING_H */
