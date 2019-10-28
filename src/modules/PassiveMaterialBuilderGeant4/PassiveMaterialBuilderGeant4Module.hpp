/**
 * @file
 * @brief Definition of Geant4 passive material construction module
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_MODULE_H
#define ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_MODULE_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

class G4RunManager;

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to construct the passive materials from the passive material file
     *
     * Creates the passvie materials from an internal configuration file and adds them to the world. The geometry that is
     * eventually constructed is used to
     * simulate the charge deposition in the \ref DepositionGeant4Module.
     */
    class PassiveMaterialBuilderGeant4Module : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        PassiveMaterialBuilderGeant4Module(Configuration& config, Messenger*, GeometryManager* geo_manager);

        /**
         * @brief Initializes Geant4 and construct the passive materials
         */
        void init() override;

    private:
        GeometryManager* geo_manager_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_MODULE_H */
