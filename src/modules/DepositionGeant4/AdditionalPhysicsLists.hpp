/**
 * @file
 * @brief Handler for implementation of additional PhysicsLists included in AllPix2 but not in the G4PhysListFactory.
 *
 * @copyright Copyright (c) 2023-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_ADDITIONAL_PHYSICS_LISTS_H
#define ALLPIX_ADDITIONAL_PHYSICS_LISTS_H

#include "core/utils/text.h"

#include <G4VModularPhysicsList.hh>

#ifdef ALLPIX_PHYSICSLIST_MICROELEC
#include <MicroElecSiPhysics.hh>
#endif

/**
 * @brief Handler namespace for implementing additional PhysicsLists included in AllPix2 but not in the
 * G4PhysListFactory.
 */
namespace allpix::physicslists {

    /**
     * @brief Function to obtain
     * @param  list_name    Name of the additional physics list
     *
     * @note Name check is performed with provided case and upper-case list names
     *
     * @return              Pointer to the G4VModularPhysicsList of the found physics list, or a nullptr if not found.
     */
    inline G4VModularPhysicsList* getList(const std::string& list_name) {

#ifdef ALLPIX_PHYSICSLIST_MICROELEC
        if(list_name == "MICROELEC-SIONLY" || allpix::transform(list_name, ::toupper) == "MICROELEC-SIONLY") {
            // Downcasting from a G4VUserPhysicsList* to a G4VModularPhysicsList
            return dynamic_cast<G4VModularPhysicsList*>(new MicroElecSiPhysics());
        }
#endif

        return nullptr;
    }

} // namespace allpix::physicslists

#endif
