/**
 * @file
 * @brief Constructs the sensitive detector and the magnetic field.
 *
 * @copyright Copyright (c) 2019-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H
#define ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H
#include "tools/geant4/SensitiveDetectorAndFieldConstruction.hpp"

namespace allpix {
    class DepositionGeant4Module;
    /**
     * @brief User hook to construct the sensitive detector and magnetic field.
     */
    class SDAndFieldConstruction : public SensitiveDetectorAndFieldConstruction {
    public:
        explicit SDAndFieldConstruction(DepositionGeant4Module* module) : module_(module){};

        /**
         * @brief Constructs the SD and field.
         */
        void ConstructSDandField() override;

    private:
        DepositionGeant4Module* module_;
    };
} // namespace allpix

#endif /* ALLPIX_DETECTOR_CONSTRUCTION_H */
