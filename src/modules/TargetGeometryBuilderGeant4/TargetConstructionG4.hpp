/**
 * @file
 * @brief Defines the internal Geant4 geometry construction
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_TARGET_CONSTRUCTION_H
#define ALLPIX_MODULE_TARGET_CONSTRUCTION_H

#include <memory>
#include <utility>
#include <G4LogicalVolume.hh>
#include "G4Material.hh"
#include "G4VSolid.hh"
#include "core/geometry/Builder.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/config/ConfigReader.hpp"

//class G4LogicalVolume;
//class G4Material;

namespace allpix {
    /**
     * @brief Constructs the Geant4 geometry during Geant4 initialization
     */
    class TargetConstructionG4 : public Builder {
    public:
        /**
         * @brief Constructs geometry construction module
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         * @param config Configuration object of the geometry builder module
         */
        TargetConstructionG4(Configuration& config);//, ConfigReader& reader);


        /**
         * @brief Constructs the world geometry with all detectors
         * @return Physical volume representing the world
         */
        void Build(void* world, void* materials) override;

    private:
        Configuration& config_;
      	//ConfigReader& reader_;
    
        // Storage of internal objects
        std::vector<std::shared_ptr<G4VSolid>> solids_;
	G4Material* world_material_{};        
    };


/*    explicit TargetConstructionG4(Configuration& config, ConfigReader reader);
    virtual ~TargetConstructionG4() = default;
        TargetConstructionG4(const TargetConstructionG4&) = default;
        TargetConstructionG4& operator=(const TargetConstructionG4&) = default;

        TargetConstructionG4(TargetConstructionG4&&) = default;
        TargetConstructionG4& operator=(TargetConstructionG4&&) = default;
*/


} // namespace allpix

#endif /* ALLPIX_MODULE_TARGET_CONSTRUCTION_H */
