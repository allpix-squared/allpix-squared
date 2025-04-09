/**
 * @file
 * @brief Definition of PropagationMapWriter module
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Please refer to the User Manual for more details on the different files of a module and the methods of the module class..
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelCharge.hpp"

#include "PropagationMap.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to generate and write pre-calculated propagation maps relating deposition position & assigned pixel
     */
    class PropagationMapWriterModule : public Module {
    public:
        /**
         * @brief Constructor
         *
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        PropagationMapWriterModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialization method of the module
         */
        void initialize() override;

        /**
         * @brief Run method of the module
         *
         * Accumulates all events into a global map
         */
        void run(Event* event) override;

        /**
         * @brief Finalization method of the module
         *
         * Generates the final mapping from the accumulated data and writes it to a field file
         */
        void finalize() override;

    private:
        // Pointers to the central geometry manager and the messenger for interaction with the framework core:
        std::shared_ptr<Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        Messenger* messenger_;

        std::array<double, 3> size_;
        std::array<size_t, 3> bins_;
        std::unique_ptr<PropagationMap> output_map_;
    };
} // namespace allpix
