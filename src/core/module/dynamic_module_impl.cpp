/**
 * @file
 * @brief Special file automatically included in the modules for the dynamic loading
 *
 * Needs the following names to be defined by the build system
 * - ALLPIX_MODULE_NAME: name of the module
 * - ALLPIX_MODULE_HEADER: name of the header defining the module
 * - ALLPIX_MODULE_UNIQUE: true if the module is unique, false otherwise
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_NAME
#error "This header should only be automatically included during the build"
#endif

#include <memory>
#include <utility>

#include "core/config/Configuration.hpp"
#include "core/geometry/Detector.hpp"
#include "core/utils/log.h"

#include ALLPIX_MODULE_HEADER

namespace allpix {
    class Messenger;
    class GeometryManager;

    extern "C" {
    /**
     * @brief Returns the type of the Module it is linked to
     *
     * Used by the ModuleManager to determine if it should instantiate a single module or modules per detector instead.
     */
    bool allpix_module_is_unique();

#if ALLPIX_MODULE_UNIQUE || defined(DOXYGEN)
    /**
     * @brief Instantiates an unique module
     * @param config Configuration for this module
     * @param messenger Pointer to the Messenger (guaranteed to be valid until the module is destructed)
     * @param geo_manager Pointer to the global GeometryManager (guaranteed to be valid until the module is destructed)
     * @return Instantiation of the module
     *
     * Internal method for the dynamic loading in the ModuleManager. Forwards the supplied arguments to the constructor and
     * returns an instantiation.
     */
    Module* allpix_module_generator(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);
    Module* allpix_module_generator(Configuration& config, Messenger* messenger, GeometryManager* geo_manager) {
        auto module = new ALLPIX_MODULE_NAME(config, messenger, geo_manager); // NOLINT
        return static_cast<Module*>(module);
    }

    // Returns that is a unique module
    bool allpix_module_is_unique() { return true; }
#endif

#if !ALLPIX_MODULE_UNIQUE || defined(DOXYGEN)
    /**
     * @brief Instantiates a detector module
     * @param config Configuration for this module
     * @param messenger Pointer to the Messenger (guaranteed to be valid until the module is destructed)
     * @param detector Pointer to the Detector object this module is bound to
     * @return Instantiation of the module
     *
     * Internal method for the dynamic loading in the ModuleManager. Forwards the supplied arguments to the constructor and
     * returns an instantiation
     */
    Module* allpix_module_generator(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);
    Module*
    allpix_module_generator(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector) { // NOLINT
        auto module = new ALLPIX_MODULE_NAME(config, messenger, std::move(detector));                          // NOLINT
        return static_cast<Module*>(module);
    }

    // Returns that is a detector module
    bool allpix_module_is_unique() { return false; }
#endif
    }
} // namespace allpix
