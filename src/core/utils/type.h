/**
 * @file
 * @brief Tags for type dispatching and run time type identification
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_TYPE_H
#define ALLPIX_TYPE_H

#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#include <string>

namespace allpix {
    /**
     * @brief Tag for specific type
     * @note This tag is needed in the \ref ::allpix namespace due to ADL lookup
     */
    template <typename T> struct type_tag {};
    /**
     * @brief Empty tag
     * @note This tag is needed in the \ref ::allpix namespace due to ADL lookup
     */
    struct empty_tag {};

#ifdef __GNUG__
    // Only demangled for GNU compiler
    inline std::string demangle(const char* name, bool keep_allpix = false) {
        // Try to demangle
        int status = -1;
        const std::unique_ptr<char, void (*)(void*)> res{abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};

        if(status == 0) {
            // Remove allpix tag if necessary
            std::string str = res.get();
            if(!keep_allpix && str.starts_with("allpix::")) {
                return str.substr(8);
            }
            return str;
        }
        return name;
    }

#else
    /**
     * @brief Demangle the type to human-readable form if it is mangled
     * @param name The possibly mangled name
     * @param keep_allpix If true the allpix namespace tag will be kept, otherwise it is removed
     */
    inline std::string demangle(const char* name, bool keep_allpix = false) { return name; }
#endif
} // namespace allpix

#endif /* ALLPIX_TYPE_H */
