/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */
#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_MODEL_G4_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_MODEL_G4_H

#include "BumpsParameterizationG4.hpp"

namespace allpix {

    // FIXME: we should either both make the detector model (PixelDetectorModel) classes or both structs
    struct DetectorModelG4 {
        // Wrapper for the whole detector in the world model (invisible)
        G4LogicalVolume* wrapper_log;
        G4VPhysicalVolume* wrapper_phys;

        // Volume containing the PCB for all sensors (green)
        G4LogicalVolume* PCB_log;
        G4VPhysicalVolume* PCB_phys;

        // Volume containing the sensitive sensors (blue)
        G4LogicalVolume* box_log;
        G4VPhysicalVolume* box_phys;

        // Volume containing the chips for each sensor (gray)
        G4LogicalVolume* chip_log;
        G4VPhysicalVolume* chip_phys;

        // Volume box containing the bumps between the sensor and the chip (yellow)
        G4LogicalVolume* bumps_log;
        G4VPhysicalVolume* bumps_phys;

        // G4LogicalVolume*    bumps_slice_log;
        // Link to replicas of the individual bonds in bumps volume box (used with the parameterization)
        G4LogicalVolume* bumps_cell_log;

        // Volume containing the guard rings around the sensor (green)
        G4LogicalVolume* guard_rings_log;
        G4VPhysicalVolume* guard_rings_phys;

        // G4LogicalVolume*    coverlayer_log;
        // G4VPhysicalVolume*  coverlayer_phys;

        // A row containing multiple replicas of pixels in a set of slices
        G4LogicalVolume* slice_log;
        // A list of cells containing a single pixel in in a slice (in the sensitive sensor box)
        G4LogicalVolume* pixel_log;

        // FIXME: do we want to save this one?
        BumpsParameterizationG4* parameterization_;
    };
}

#endif
