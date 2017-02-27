/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */
#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_MODEL_G4_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_MODEL_G4_H

#include "BumpsParameterizationG4.hpp"

namespace allpix {
    
    // FIXME: we should either both make the detector model (PixelDetectorModel) classes or both structs
    struct DetectorModelG4 {
        G4LogicalVolume*    wrapper_log;
        G4VPhysicalVolume*  wrapper_phys;
        
        G4LogicalVolume*    PCB_log;
        G4VPhysicalVolume*  PCB_phys;
        
        G4LogicalVolume*    box_log;
        G4VPhysicalVolume*  box_phys;
        
        //G4LogicalVolume*    coverlayer_log;
        //G4VPhysicalVolume*  coverlayer_phys;
        
        G4LogicalVolume*    chip_log;
        G4VPhysicalVolume*  chip_phys;
        
        G4LogicalVolume*    bumps_log;
        G4VPhysicalVolume*  bumps_phys;
        
        //G4LogicalVolume*    bumps_slice_log;
        G4LogicalVolume*    bumps_cell_log;
        
        G4LogicalVolume*    guard_rings_log;
        G4VPhysicalVolume*  guard_rings_phys;
        
        G4LogicalVolume*    slice_log;
        G4LogicalVolume*    pixel_log;
        
        // FIXME: do we want to save this one?
        BumpsParameterizationG4 *parameterization_;
    };
}

#endif
