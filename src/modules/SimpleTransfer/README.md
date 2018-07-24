# SimpleTransfer
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Input**: PropagatedCharge  
**Output**: PixelCharge  

### Description
Combines individual sets of propagated charges together to a set of charges on the sensor pixels and thus prepares them for processing by the detector front-end electronics. The module does a simple direct mapping to the nearest pixel, ignoring propagated charges that are too far away from the implants or outside the pixel grid. Timing information for the pixel charges is currently not yet produced, but can be fetched from the linked propagated charges.

A histogram of charge carrier arrival times is generated if `output_plots` is enabled. The range and granularity of this plot can be configured.

### Parameters
* `max_depth_distance` : Maximum distance in depth, i.e. normal to the sensor surface at the implant side, for a propagated charge to be taken into account. Defaults to `5um`.
* `output_plots`: Determines if output plots should be generated. Disabled by default.
* `output_plots_step`: Bin size of the arrival time histogram in units of time. Defaults to `0.1ns`.
* `output_plots_range`: Total range of the arrival time histogram. Defaults to `100ns`.

### Usage
For a typical simulation, a *max_depth_distance* a few micro meters should be sufficient, leading to the following configuration:

```ini
[SimpleTransfer]
max_depth_distance = 5um
```
