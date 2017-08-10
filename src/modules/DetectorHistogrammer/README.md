## DetectorHistogrammer
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>), Paul Schuetze (<paul.schuetze@desy.de>)
**Status**: Functional (candidate for removal)  
**Input**: PixelHit  

#### Description
Creates a hitmap of all the pixels in the pixel grid, displaying the number of times a pixel has been hit during the simulation run, and a cluster map for the whole simulation run. Also creates histograms of the cluster sizes in x, y and total for every cluster and the total number of hits for each event. Should only be used for quick inspection and checks. For more sophisticated analyses the output from one of the output writers should be used to produce the necessary information. 

#### Parameters
*No parameters*

#### Usage
This module is normally bound to a specific detector to plot, for example to the 'dut':

```ini
[DetectorHistogrammer]
name = "dut"
```
