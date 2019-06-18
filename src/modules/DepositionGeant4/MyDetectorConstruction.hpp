/**
 * @file
 * @brief
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H
#define ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H
#include "G4RunManager/DetectorConstruction.hpp"

namespace allpix {
    class DepositionGeant4Module;
    /**
     * @brief
     */
    class MyDetectorConstruction: public DetectorConstruction {
    public:
        MyDetectorConstruction(DepositionGeant4Module* module, double fano_factor, double charge_creation_energy)
            : module_(module), fano_factor_(fano_factor), charge_creation_energy_(charge_creation_energy){};

        virtual void ConstructSDandField() override;
    private:
        DepositionGeant4Module* module_;
        double fano_factor_;
        double charge_creation_energy_;
    };
} // namespace allpix

#endif /* ALLPIX_DETECTOR_CONSTRUCTION_H */
