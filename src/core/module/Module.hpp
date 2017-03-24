/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_H
#define ALLPIX_MODULE_H

#include <string>
#include <vector>

#include "ModuleIdentifier.hpp"

#include "core/geometry/Detector.hpp"

namespace allpix {

    class Messenger;

    class Module {
    public:
        // Constructor and destructors
        explicit Module();
        explicit Module(std::shared_ptr<Detector>);
        virtual ~Module() = default;

        // Disallow copy of a module
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;

        // Modules should have a unique name (for configuration)
        // TODO: depends on implementation how this should work with dynamic loading
        // virtual std::string getName() = 0;

        // Get (possibly) linked detector
        std::shared_ptr<Detector> getDetector();

        // Initialize the module and pass the configuration etc.
        virtual void init() {
            // FIXME: do default stuff here
        }

        // Execute the function of the module
        virtual void run() = 0;

        // Finalize module and check if it executed properly
        // NOTE: useful to do before destruction to allow for exceptions
        virtual void finalize() {
            // FIXME: do default stuff here
        }

    private:
        std::shared_ptr<Detector> detector_;
    };
}

#endif /* ALLPIX_MODULE_MANAGER_H */
