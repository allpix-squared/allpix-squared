/**
 * @file
 * @brief Parameters of a box passive material model
 *
 * @copyright Copyright (c) 2021-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_GDML_H
#define ALLPIX_PASSIVE_MATERIAL_GDML_H

#ifdef Geant4_GDML

#include <string>

#include <G4Color.hh>
#include <G4GDMLParser.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4SubtractionSolid.hh>
#include <G4VSolid.hh>
#include <G4VisAttributes.hh>

#include "PassiveMaterialModel.hpp"
#include "tools/geant4/geant4.h"

namespace allpix {

    /**
     * @ingroup PassiveMaterialModel
     * @brief Model for passive material loaded from GDML files
     */
    class GDMLModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the box passive material model
         * @param config Configuration with description of the model
         * @param geo_manager Pointer to the global geometry manager
         */
        explicit GDMLModel(const Configuration& config, GeometryManager* geo_manager)
            : PassiveMaterialModel(config, geo_manager) {

            auto gdml_file = config_.getPath("file_name");
            parser_.ReadModule(gdml_file.string(), false);

            // Adding points to extend world volume as much as necessary
            LOG(DEBUG) << "Adding points for volume";
            add_points();
        }

        /**
         * @brief Method to build the volumes and add it to the world. Overrides the parent class method to be able to handle
         * complex structures with may solids from GDML files
         * @param world_log Pointer to the world volume
         */
        void buildVolume(const std::shared_ptr<G4LogicalVolume>& world_log) override {

            LOG(TRACE) << "Building passive material: " << getName();
            G4LogicalVolume* mother_log_volume = nullptr;
            if(!getMotherVolume().empty()) {
                auto* log_volume_store = G4LogicalVolumeStore::GetInstance();
                mother_log_volume = log_volume_store->GetVolume(getMotherVolume() + "_log");
            } else {
                mother_log_volume = world_log.get();
            }

            if(mother_log_volume == nullptr) {
                throw InvalidValueError(config_, "mother_volume", "mother_volume does not exist");
            }

            std::vector<std::string> name_list; // Contains the names of the daughter volumes
            auto* gdml_world_phys = parser_.GetWorldVolume();
            auto* gdml_world_log = gdml_world_phys->GetLogicalVolume();

            bool color_from_gdml = false;
            LOG(DEBUG) << "Total number of daughter volumes: " << gdml_world_log->GetNoDaughters();
            while(gdml_world_log->GetNoDaughters() > 0) {
                LOG(TRACE) << "Current number of daughter volumes left: " << gdml_world_log->GetNoDaughters();
                G4VPhysicalVolume* gdml_daughter = gdml_world_log->GetDaughter(0);
                G4LogicalVolume* gdml_daughter_log = gdml_daughter->GetLogicalVolume();

                // Remove the daughter from its world volume in order to add it to the global one
                gdml_world_log->RemoveDaughter(gdml_daughter);

                std::string gdml_daughter_name = gdml_daughter->GetName();
                if(std::find(name_list.begin(), name_list.end(), gdml_daughter_name) != name_list.end()) {
                    gdml_daughter_name += "_";
                    gdml_daughter->SetName(gdml_daughter_name);
                    gdml_daughter->SetCopyNo(gdml_daughter->GetCopyNo() + 1);
                    gdml_daughter_log->SetName(gdml_daughter_name);
                }

                LOG(DEBUG) << "Volume " << name_list.size() << ": " << gdml_daughter_name;
                name_list.push_back(gdml_daughter_name);

                // Add offset and rotation to current daughter location
                G4ThreeVector position_vector = toG4Vector(position_);
                auto* daughter_rotation = gdml_daughter->GetRotation();
                // It seems that in the case of a trivial rotation the daughter_rotation can be a nullptr
                auto* rotation_matrix =
                    ((daughter_rotation != nullptr) ? new G4RotationMatrix(*daughter_rotation * *rotation_)
                                                    : new G4RotationMatrix(*rotation_));
                LOG(TRACE) << "Rotation matrix: " << *rotation_matrix;
                gdml_daughter->SetTranslation((rotation_->inverse()) * gdml_daughter->GetTranslation() + position_vector);
                gdml_daughter->SetRotation(rotation_matrix);

                // Check if color information is available and set it to the daughter volume
                for(auto& aux : parser_.GetVolumeAuxiliaryInformation(gdml_daughter_log)) {
                    std::transform(aux.type.begin(), aux.type.end(), aux.type.begin(), ::tolower);
                    if(aux.type == "color" || aux.type == "colour") {
                        gdml_daughter_log->SetVisAttributes(G4VisAttributes(get_color(aux.value)));
                        color_from_gdml = true;
                    }
                }

                // Check if there was color information in the configuration:
                if(config_.has("color") && !color_from_gdml) {
                    set_visualization_attributes(gdml_daughter_log, mother_log_volume); // NOLINT
                }

                // Add the physical daughter volume to the mother volume and configure logical mother volume
                mother_log_volume->AddDaughter(gdml_daughter);
                gdml_daughter->SetMotherLogical(mother_log_volume);
            }

            if(config_.has("color") && color_from_gdml) {
                LOG(INFO) << "Configured visualization attributes of passive material \"" << getName()
                          << "\" was partially overwritten by GDML information";
            }
        };

        // Provide maximum extend of this model by looking at the GDML world volume
        double getMaxSize() const override {
            auto* world = parser_.GetWorldVolume();
            auto* box = dynamic_cast<G4Box*>(world->GetLogicalVolume()->GetSolid());
            if(box == nullptr) {
                throw InvalidValueError(config_, "file_name", "Could not deduce world size from GDML file");
            }
            return 2. * std::max({box->GetXHalfLength(), box->GetYHalfLength(), box->GetZHalfLength()});
        }

    private:
        std::shared_ptr<G4VSolid> get_solid() const override { return nullptr; }

        G4GDMLParser parser_;

        /**
         * @brief Retrieve and parse color value from GDML file
         * @param  value Color representation as string
         * @return Color value as G4COlor
         */
        G4Colour get_color(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);

            int r = 256, g = 256, b = 256, a = 256;
            if(value.size() >= 6) {
                // Value contains RGBA color
                value.erase(std::remove(value.begin(), value.end(), '#'), value.end());
                std::istringstream(value.substr(0, 2)) >> std::hex >> r;
                std::istringstream(value.substr(2, 2)) >> std::hex >> g;
                std::istringstream(value.substr(4, 2)) >> std::hex >> b;
                if(value.size() >= 8) {
                    std::istringstream(value.substr(6, 2)) >> std::hex >> a;
                }
            }

            // If no valid color code was specified, return white
            return {static_cast<double>(r) / 256,
                    static_cast<double>(g) / 256,
                    static_cast<double>(b) / 256,
                    static_cast<double>(a) / 256};
        };
    };
} // namespace allpix

#endif /* Geant4_GDML */
#endif /* ALLPIX_PASSIVE_MATERIAL_GDML_H */
