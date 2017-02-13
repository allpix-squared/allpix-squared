/**
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 */

#include "string.h"

using namespace allpix;

inline std::string trim(const std::string &s){
    static const std::string spaces = " \t\n\r\v";
    size_t b = s.find_first_not_of(spaces);
    size_t e = s.find_last_not_of(spaces);
    if (b == std::string::npos || e == std::string::npos) {
        return "";
    }
    return std::string(s, b, e - b + 1);
}
