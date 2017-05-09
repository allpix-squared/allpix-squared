/**
 * @file
 * Special file automatically included in the modules for the dynamic loading
 *
 * Needs the following names to be defined by the build system
 * - ALLPIX_MODULE_NAME: name of the module
 * - ALLPIX_MODULE_HEADER: name of the header defining the module
 * - ALLPIX_MODULE_UNIQUE: true if the module is unique, false otherwise
 *
 * @copyright MIT License
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
     * @brief Instantiates a unique module
     *
     * Forwards the supplied arguments to the constructor and returns an instantiation
     */
    Module* allpix_module_generator(Configuration config, Messenger* messenger, GeometryManager* geometry);
    Module* allpix_module_generator(Configuration config, Messenger* messenger, GeometryManager* geometry) {
        auto module = new ALLPIX_MODULE_NAME(std::move(config), messenger, geometry); // NOLINT
        return static_cast<Module*>(module);
    }

    // Returns that is a unique module
    bool allpix_module_is_unique() { return true; }
#endif

#if !ALLPIX_MODULE_UNIQUE || defined(DOXYGEN)
    /**
     * @brief Instantiates a detector module
     *
     * Forwards the supplied arguments to the constructor and returns an instantiation
     */
    Module* allpix_module_generator(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector);
    Module* allpix_module_generator(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector) {
        auto module = new ALLPIX_MODULE_NAME(std::move(config), messenger, std::move(detector)); // NOLINT
        return static_cast<Module*>(module);
    }

    // Returns that is a detector module
    bool allpix_module_is_unique() { return false; }
#endif
    }
}
