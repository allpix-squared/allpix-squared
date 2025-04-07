/**
 * @file
 * @brief Definition of PixESLWriter module
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <string>

#include <libapx/writer.hpp>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelHit.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module which writes out data for the PixESL framework
     */
    class PixESLWriterModule : public SequentialModule {
    public:
        /**
         * @brief Constructor
         *
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        PixESLWriterModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialization method
         */
        void initialize() override;

        /**
         * @brief Run method
         */
        void run(Event* event) override;

    private:
        // Pointers to the central geometry manager and the messenger for interaction with the framework core:
        std::shared_ptr<Detector> detector_;
        Messenger* messenger_;

        std::unique_ptr<apx::Writer> writer_;
    };
} // namespace allpix
