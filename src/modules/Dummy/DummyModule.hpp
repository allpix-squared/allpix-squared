/**
 * @file
 * @brief Definition of [Dummy] module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Please refer to the User Manual for more details on the different files of a module and the methods of the module class..
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelHit.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module which serves as a demonstrator and wireframe for new modules
     *
     * This module is only a dummy and here to demonstrate the general structure of a module with its different member
     * methods, the messenger and event interfaces. It also serves as wireframe for new modules, which can take the structure
     * for a quicker start.
     */
    class DummyModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         *
         * The constructor of the module is called right after the module has been instantiated. It has access to the module
         * configuration and can set defaults and retriebe values from the config. The constructor is also the place where
         * multithreading capabilities of the module need to be announced, and where the messenger should be notified about
         * desired input messages that should be relayed to this module.
         *
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DummyModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initialization method of the module
         *
         * This method is called during the initialization phase of the framework. In this method, all necessary setup steps
         * for this module should be conducted, such that the module is ready to perform simulations. Typically at this stage
         * additional checks on compatibility of the user-configured parameters and the information such as fields and maps
         * obtained from the detector models are implemented.
         *
         * This method is called once per simulation run, before the event loop is started.
         *
         * Implementing this method is optional, if no initialization is required there is no need to override and implement
         * this method.
         */
        void initialize() override;

        /**
         * @brief Run method of the module
         *
         * This is the event processing method of the module and is called for every single event in the event loop once. The
         * method should retrieve potentially registered messages, process them, dispatch possible output messages to the
         * messenger of the framework, and the return control to the caller by ending the method.
         */
        void run(Event* event) override;

        /**
         * @brief Finalization method of the module
         *
         * In this method, finalization steps of the module can be performed after the event loop has been finished. This
         * could for example comprise aggregation of final histograms, the calculation of a final value averaged over all
         * events, or the closing of an output file.
         *
         * This method is called once per simulation run, after the event loop has been finished.
         *
         * Implementing this method is optional, if no finalization is required there is no need to override and implement
         * this method.
         */
        void finalize() override;

    private:
        // Pointers to the central geometry manager and the messenger for interaction with the framework core:
        GeometryManager* geo_manager_;
        Messenger* messenger_;

        // A local module member variable which is set on the constructor and only read during runtime:
        int setting_{};
    };
} // namespace allpix
