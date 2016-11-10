#ifndef ALLPIX_INCLUDED_Utils
#define ALLPIX_INCLUDED_Utils

#include <string>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <vector>

namespace allpix {

  std::string trim(const std::string &s);
  
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

  template <>
  inline std::string
  from_string(const std::string &x, const std::string &def) {
    return x == "" ? def : x;
  }

  template <>
  int64_t from_string(const std::string &x, const int64_t &def);

  template <>
  uint64_t from_string(const std::string &x, const uint64_t &def);

  template <>
  inline int32_t
  from_string(const std::string &x, const int32_t &def) {
    return static_cast<int32_t>(from_string(x, (int64_t)def));
  }

  template <>
  inline uint32_t from_string(const std::string &x, const uint32_t &def) {
    return static_cast<uint32_t>(from_string(x, (uint64_t)def));
  }

  
  /** Converts any type to a string.
   * \param x The value to be converted.
   * \return A string representing the passed in parameter.
   */
  template <typename T>
  inline std::string to_string(const T &x, int digits = 0) {
    std::ostringstream s;
    s << std::setfill('0') << std::setw(digits) << x;
    return s.str();
  }

  template <typename T>
  inline std::string to_string(const std::vector<T> &x, const std::string &sep,
                               int digits = 0) {
    std::ostringstream s;
    if (x.size() > 0)
      s << to_string(x[0], digits);
    for (size_t i = 1; i < x.size(); ++i) {
      s << sep << to_string(x[i], digits);
    }
    return s.str();
  }

  template <typename T>
  inline std::string to_string(const std::vector<T> &x, int digits = 0) {
    return to_string(x, ",", digits);
  }

  inline std::string to_string(const std::string &x, int /*digits*/ = 0) {
    return x;
  }

  inline std::string to_string(const char *x, int /*digits*/ = 0) { return x; }

}

#endif // ALLPIX_INCLUDED_Utils
