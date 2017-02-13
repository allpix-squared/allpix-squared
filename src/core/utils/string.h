/**
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 */

#ifndef ALLPIX_STRING_H
#define ALLPIX_STRING_H

#include <string>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <vector>

namespace allpix {
    
    /** Trims the leading and trailing white space from a string
     */
    inline std::string trim(const std::string &s);
    
    /** Converts a string to any type.
     * \param x The string to be converted.
     * \param def The default value to be used in case of an invalid string,
     *            this can also be useful to select the correct template type
     *            without having to specify it explicitly.
     * \return An object of type T with the value represented in x, or if
     *         that is not valid then the value of def.
     */
    template <typename T>
    inline T from_string(const std::string &x, const T &def = 0) {
        if (x == "")
            return def;
        T ret = def;
        std::istringstream s(x);
        s >> ret;
        char remain = '\0';
        s >> remain;
        if (remain)
            throw std::invalid_argument("Invalid argument: " + x);
        return ret;
    }
    
    /** Splits string s into elements at delimiter "delim" and returns them as vector
     */
    template <typename T>
    std::vector<T> &split(const std::string &s, std::vector<T> &elems, char delim) {
        
        // If the input string is empty, simply return the default elements:
        if (s.empty()) return elems;
        
        // Else we have data, clear the default elements and chop the string:
        elems.clear();
        std::stringstream ss(s);
        std::string item;
        T def;
        while (std::getline(ss, item, delim)) {
            elems.push_back(from_string(item, def));
        }
        return elems;
    }
    
}

#endif // ALLPIX_INCLUDED_Utils
