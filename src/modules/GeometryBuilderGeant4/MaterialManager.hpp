/**
 * @file
 * @brief Light-weigth material manager for the GeometryBuilderGeant4 module
 *
 * @copyright Copyright (c) 2021-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_GEANT4_MATERIALS_H
#define ALLPIX_GEANT4_MATERIALS_H

#include "core/utils/log.h"

#include <G4Material.hh>
#include <G4NistManager.hh>

namespace allpix {

    /**
     * Singleton class to manage materials
     *
     * This manager both holds often-used, pre-defined materials and provides access to the Geant4 NIST database of
     * materials. It is a singleton class and can be extended at run time via the \ref set method.
     */
    class Materials {
    public:
        Materials(Materials const&) = delete;
        void operator=(Materials const&) = delete;

        static Materials& getInstance();

        /**
         * Method to get a Geant4 material from the manager's internal database or from external sources
         * @param material Material name
         * @returns Material
         * @throws ModuleError if material cannot be found
         */
        G4Material* get(const std::string& material) const;

        /**
         * Method to add an additional material to the internal database
         * @param name Name of the material to be registered
         * @param material Pointer to the material object
         */
        void set(const std::string& name, G4Material* material);

    private:
        /**
         * Private constructor for singleton class
         */
        Materials() { init_materials(); };

        /**
         * @brief Initializes the list of materials with pre-defined materials of the framework
         */
        void init_materials();

        // Map of materials
        std::map<std::string, G4Material*> materials_;
    };

} // namespace allpix

#endif /* ALLPIX_GEANT4_MATERIALS_H */
