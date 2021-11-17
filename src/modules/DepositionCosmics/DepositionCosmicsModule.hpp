/**
 * @file
 * @brief Definition of [DepositionCosmics] module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "../DepositionGeant4/DepositionGeant4Module.hpp"

#include "objects/PixelHit.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class DepositionCosmicsModule : public DepositionGeant4Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionCosmicsModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

    private:
        void initialize_g4_action() override;

        // Config parameter for data file paths:
        std::vector<std::string> data_paths_;
    };
} // namespace allpix
