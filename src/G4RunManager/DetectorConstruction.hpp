/**
 * @file
 * @brief
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_DETECTOR_CONSTRUCTION_H
#define ALLPIX_DETECTOR_CONSTRUCTION_H


namespace allpix {
    /**
     * @brief Constructs the Geant4 geometry during Geant4 initialization
     */
    class DetectorConstruction {
    public:

        virtual void ConstructSDandField() {
            
        }
    };
} // namespace allpix

#endif /* ALLPIX_DETECTOR_CONSTRUCTION_H */
