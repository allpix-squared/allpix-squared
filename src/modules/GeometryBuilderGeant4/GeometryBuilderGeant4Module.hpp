/**
 * @file
 * @brief Definition of Geant4 geometry construction module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_MODULE_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_MODULE_H

#include <memory>
#include <string>

#include "GeometryConstructionG4.hpp"
#include "PassiveMaterialConstructionG4.hpp"
#include "core/config/ConfigReader.hpp"
#include "core/config/Configuration.hpp"

#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

class G4RunManager;

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to construct the Geant4 geometry from the internal geometry
     *
     * Creates the world from the information available from the \ref GeometryManager. Then continues with constructing every
     * detector, building it from the internal detector model. The geometry that is eventually constructed is used to
     * simulate the charge deposition in the \ref DepositionGeant4Module.
     */
    class GeometryBuilderGeant4Module : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        GeometryBuilderGeant4Module(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initializes Geant4 and construct the Geant4 geometry from the internal geometry
         */
        void initialize() override;

    private:
        GeometryManager* geo_manager_;
        // Geant4 run manager is owned by this module
        GeometryConstructionG4* geometry_construction_;
        std::unique_ptr<G4RunManager> run_manager_g4_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_MODULE_H */
