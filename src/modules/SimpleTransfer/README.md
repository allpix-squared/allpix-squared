## SimpleTransfer
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Input**: PropagatedCharge  
**Output**: PixelCharge  

#### Description
Combines individual sets of propagated charges together to a set of charges on the sensor pixels. The module does a simple direct mapping to the nearest pixel, ignoring propagated charges that are to far away from the implants or outside the pixel grid. Timing information for the pixel charges is currently not yet produced.

#### Parameters
* `max_depth_distance` : Maximum distance in the depth direction (normal to the pixel grid) from the implant side for a propagated charge to be taken into account.

#### Usage
For typical simulation purposes a *max_depth_distance* around 10um should be sufficient, leading to the following configuration:

```ini
[SimpleTransfer]
max_depth_distance = 10um
```
