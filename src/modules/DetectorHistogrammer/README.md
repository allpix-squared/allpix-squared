## DetectorHistogrammer
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>), Paul Schuetze (<paul.schuetze@desy.de>)
**Status**: Functional   
**Input**: PixelHit

#### Description
This module should only give an overview of the produced simulation data for a quick inspection and simple checks. For more sophisticated analyses the output from one of the output writers should be used to produce the necessary information.

Within the module, a simple clustering is performed. Looping over the PixelHits, hits being adjacent to an existing cluster, are added to this cluster. If not, a new cluster is created.

The module creates the following histograms:

* A hitmap of all pixels in the pixel grid, displaying the number of times a pixel has been hit during the simulation run.
* A cluster map indicating the cluster positions for the whole simulation run.
* Total number of pixel hits (event size) per event (an event can have multiple particles).
* Cluster sizes in x, y and total per cluster.

#### Parameters
*No parameters*

#### Usage
This module is normally bound to a specific detector to plot, for example to the 'dut':

```ini
[DetectorHistogrammer]
name = "dut"
```
