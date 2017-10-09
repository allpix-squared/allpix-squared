## CapacitiveTransfer
**Maintainer**: Mateus Vicente (mvicente@cern.ch)  
**Status**: Functional (WIP)  
**Input**: *PropagatedCharge*  
**Output**: *PixelCharge*  

#### Description
As the SimpleTransferModule, this module Combines individual sets of propagated charges together to a set of charges on the sensor pixels and thus prepares them for processing by the detector front-end electronics. In addition to the SimpleTransferModule, where the charge close to the implants is transferred only to the closest pixel, this module also copies the propagated charge to the sensor neighbouring pixels, scaled by the respective cross-coupling, in order to simulate the cross-coupling between neighbouring pixels in Capacitively Coupled Pixel Detectors (CCPDs). 
It is also possible to simulate assemblies with tilted chips, with non-uniform coupling over the pixel matrix, by providing thetilting angles between the chips, the nominal gap between the pixel pads, the pixel coordinates where the chips are away from each other by the nominal gap provided and a root file containing the coupling capacitances *vs* gap between pixel pads.

#### Parameters
* max_depth_distance: Maximum distance in depth, i.e. normal to the sensor surface at the implant side, for a propagated charge to be taken into account. Defaults to 5um.
* matrix_file: Path to the file containing the cross-couling matrix. The file must contain the relative capacitance to the central pixel (i.e. cross_capacitance / direct_capacitance).
* coupling_matrix: Cross-coupling matrix with relative capacitances.
* chip_angle: Tilt angle between chips. The first angle is the rotation along the columns axis,  and second is along the row axis. It defaults to 0.0 radians (parallel chips). 
* tilt_center: Pixel position for the nominal coupling/distance.
* nominal_gap: Nominal gap between chips.
* scan_file: Root file containing a TGraph, for each pixel, with the capacitance measured for each gap between the pixel pads.

The cross-coupling matrix, to be parsed via the matrix file or via the configuration file, must be square and organized in Row vs Col, such as:
 ```ini
 cross_coupling_00    cross_coupling_01    cross_coupling_02
 cross_coupling_10    cross_coupling_11    cross_coupling_12
 cross_coupling_20    cross_coupling_21    cross_coupling_22
 ```
 cross_coupling_11 is the coupling to the closest pixel and should be always 1.

#### Usage
 For a typical simulation, a *max_depth_distance* of a few micro meters is sufficient, leading to the following configuration:

 ```ini
 [CapacitiveTransfer]
 max_depth_distance = 5um
 matrix_file = "capacitance_matrix.txt"
 ```

 or
 ```ini
 [CapacitiveTransfer]
 max_depth_distance = 5um
 coupling_matrix = 0.1    0.3    0.1
 		   0.2     1     0.2
		   0.1    0.3    0.1
 ```
 
 or
 ```ini
 [CapacitiveTransfer]
 max_depth_distance = 5um
 scan_file = "capacitance_vs_gap.root"
 chip_angle = 0.0524545rad 0.0524545rad
 tilt_center = 0 0
 nominal_gap = 2um
 ```
