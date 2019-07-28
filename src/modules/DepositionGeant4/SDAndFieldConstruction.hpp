/**
 * @file
 * @brief Constructs the sensitive detector and the magnetic field.
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H
#define ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H
#include "G4RunManager/SensitiveDetectorAndFieldConstruction.hpp"

namespace allpix {
    class DepositionGeant4Module;
    /**
     * @brief User hook to construct the sensitive detector and magnetic field.
     */
    class SDAndFieldConstruction : public SensitiveDetectorAndFieldConstruction {
    public:
        SDAndFieldConstruction(DepositionGeant4Module* module, double fano_factor, double charge_creation_energy)
            : module_(module), fano_factor_(fano_factor), charge_creation_energy_(charge_creation_energy){};

        /**
         * @brief Constructs the SD and field.
         */
        virtual void ConstructSDandField() override;

    private:
        DepositionGeant4Module* module_;
        double fano_factor_;
        double charge_creation_energy_;
    };
} // namespace allpix

#endif /* ALLPIX_DETECTOR_CONSTRUCTION_H */
