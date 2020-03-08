# DepositionReader
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Status**: Functional  
**Output**: DepositedCharge

### Description
This module allows to read in energy depositions in a sensor volume produced with a different program, e.g. with GEant4 in a standalone simulation of the respective experiment.
The detector geometry for Allpix Squared should resemble the global positions of the detectors of interest in the original simulation.
The global coordinates of the external simulation are used to assign energy deposits to calculate the corresponding local coordinates.

The assignment of energy deposits to a specific detector in the Allpix Squared simulation is performed using the volume name of the detector element in the original simulation.
Hence, the naming of the detector in the geometry file has to match its name in the input data file.
In order to simplify the aggregation of individual detector element volumes from the original simulation into a single detector, this modules provides the `detector_name_chars` parameter which allows matching of the detector name to be performed on a substring of the original volume name.

Currently two data sources are supported, ROOT trees and CSV text files.
Their expected formats are explained in detail in the following.

#### ROOT Trees


#### CSV Files

Data in CSV-formatted text files are interpreted as follows.
Empty lines as well as lines starting with a hash (`#`) are ignored, all other lines are interpreted either as event header if they start with `E`, or as energy deposition:

```csv
Event: <N>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>
# ...
# For example:
211, 3.234674e+01, 1.091620e-02, -2.515335e+00, 4.427924e+00, -2.497500e-01, MyDetector

Event: <N+1>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>
# ...
```

where `<N>` is the current event number, `<PID>` is the PDG particle ID [@pdg], `<T>` the time of deposition, calculated from the beginning of the event, `<E>` is the deposited energy in units of `keV`m `<[X-Z]>` is the position of the energy deposit in global coordinates of the setup, and `<V>` the the detector name (volume) the energy deposit should be assigned to.

Only energy deposits with a valid volume are considered, i.e. where a matching detector with the same name can be found in the geometry setup.
The global coordinates are then translated to local coordinates of the given detector.
If these are outside the sensor, the energy deposit is discarded.


### Parameters
* `model`: Format of the data file to be read, can either be `csv` or `root`.
* `detector_name_chars`: Parameter which allows selecting only a substring of the stored volume name as detector name. Could be set to the number of characters from the beginning of the volume name string which should be taken as detector name. E.g. `detector_name_chars = 7` would select `sensor0` from the full volume name `sensor0_px3_14` read from the input file. This is especially useful if the initial simulation in Geant4 has been performed using parameterized volume placements e.g. for individual pixels of a detector. Defaults to `0` which takes the full volume name.

### Usage
*Example how to use this module*

[@pdg]: http://hepdata.cedar.ac.uk/lbl/2016/reviews/rpp2016-rev-monte-carlo-numbering.pdf
