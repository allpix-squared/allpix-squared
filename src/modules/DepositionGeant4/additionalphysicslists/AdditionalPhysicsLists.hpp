/**
 * @file
 * @brief Handler for implementation of additional PhysicsLists included in AllPix2 but not in the G4PhysListFactory.
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_ADDITIONAL_PHYSICS_LISTS_H
#define ALLPIX_ADDITIONAL_PHYSICS_LISTS_H

// The MicroElec PhysicsLists
#include "microelec/MicroElecPhysics.hpp"
#include "microelec/MicroElecSiPhysics.hpp"

namespace allpix {
    /**
     * @brief Handler namespace for implementing additional PhysicsLists included in AllPix2 but not in the
     * G4PhysListFactory.
     */

    namespace AdditionalPhysicsLists {

        /**
         * @brief Function to obtain
         * @param  list_name    Name of the additional physics list
         * @return              Pointer to the G4VModularPhysicsList of the found physics list, or a nullptr if not found.
         */
        G4VModularPhysicsList* getList(std::string list_name) {
            if(list_name == std::string("microelec"))
                return static_cast<G4VModularPhysicsList*>(new MicroElecPhysics());

            else if(list_name == "microelec-sionly")
                return static_cast<G4VModularPhysicsList*>(new MicroElecSiPhysics());

            return nullptr;
        }

    } // namespace AdditionalPhysicsLists
} // namespace allpix

#endif
