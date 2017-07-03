## DetectorHistogrammer
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional (candidate for removal)  
**Input**: PixelHit  

#### Description
Creates a hitmap of all the pixels in the pixel grid, displaying the number of times a pixel has been hit during the simulation run. Also creates a histogram of the cluster size for every event. Should only be used for quick inspection and checks. For more sophisticated analyses the output from one of the output writers should be used to produce the necessary information. 

#### Parameters
*No parameters*

#### Usage
This module is normally bound to a specific detector to plot, for example to the 'dut':

```ini
[DetectorHistogrammer]
name = "dut"
```
