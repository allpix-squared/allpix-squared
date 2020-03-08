# DepositionReader
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)
**Status**: Functional
**Output**: DepositedCharge

### Description
This module allows to read in energy depositions in a sensor volume produced with a different program, e.g. with GEant4 in a standalone simulation of the respective experiment.
The detector geometry for Allpix Squared should resemble the global positions of the detectors of interest in the original simulation.

The assignment of energy deposits to a specific detector in the Allpix Squared simulation is performed using the volume name of the detector element in the original simulation.
Hence, the naming of the detector in the geometry file has to match its name in the input data file.
In order to simplify the aggregation of individual detector element volumes from the original simulation into a single detector, this modules provides the `detector_name_chars` parameter which allows matching of the detector name to be performed on a substring of the original volume name.

Only energy deposits with a valid volume are considered, i.e. where a matching detector with the same name can be found in the geometry setup.
The global coordinates are then translated to local coordinates of the given detector.
If these are outside the sensor, the energy deposit is discarded and a warning is printed.
The number of electron/hole pairs created by a given energy deposition is calculated using the mean pair creation energy `charge_creation_energy`, fluctuations are modeled using a Fano factor `fano_factor` assuming Gaussian statistics.

Track and parent ids of the individual particles which created the energy depositions allow to carry on some of the Monte Carlo particle information from the original simulation.
A parent id of zero should be used for the primary particle of the simulation, and all track ids have to be recorded before they can be used as parent id.

Currently two data sources are supported, ROOT trees and CSV text files.
Their expected formats are explained in detail in the following.

#### ROOT Trees


#### CSV Files

Data in CSV-formatted text files are interpreted as follows.
Empty lines as well as lines starting with a hash (`#`) are ignored, all other lines are interpreted either as event header if they start with `E`, or as energy deposition:

```csv
Event: <N>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>,<TRK>,<PRT>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>,<TRK>,<PRT>
# ...
# For example:
211, 3.234674e+01, 1.091620e-02, -2.515335e+00, 4.427924e+00, -2.497500e-01, MyDetector, 1, 0
211, 3.234843e+01, 1.184756e-02, -2.528475e+00, 4.453544e+00, -2.445500e-01, MyDetector, 2, 1

Event: <N+1>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>,<TRK>,<PRT>
# ...
```

where `<N>` is the current event number, `<PID>` is the PDG particle ID [@pdg], `<T>` the time of deposition, calculated from the beginning of the event, `<E>` is the deposited energy in units of `keV`m `<[X-Z]>` is the position of the energy deposit in global coordinates of the setup, and `<V>` the the detector name (volume) the energy deposit should be assigned to.
`<TRK>` represents the track id of the prticle track which has caused this energy deposition, and `<PRT>` the id of the parent particle which created this particle.

### Parameters
* `model`: Format of the data file to be read, can either be `csv` or `root`.
* `file_name`: Location of the input data file. The appropriate file extension will be appended if not present, depending on the `model` chosen either `.csv` or `.root`.
* `tree_name`: Name of the input tree to be read from the ROOT file. Only used for the `root` model.
* `detector_name_chars`: Parameter which allows selecting only a substring of the stored volume name as detector name. Could be set to the number of characters from the beginning of the volume name string which should be taken as detector name. E.g. `detector_name_chars = 7` would select `sensor0` from the full volume name `sensor0_px3_14` read from the input file. This is especially useful if the initial simulation in Geant4 has been performed using parameterized volume placements e.g. for individual pixels of a detector. Defaults to `0` which takes the full volume name.
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in silicon (3.64 eV, [@chargecreation]).
* `fano_factor`: Fano factor to calculate fluctuations in the number of electron/hole pairs produced by a given energy deposition. Defaults to 0.115 [@fano].
* `unit_length`: The units length measurements read from the input data source should be interpreted in. Defaults to the framework standard unit `mm`.
* `unit_time`: The units time measurements read from the input data source should be interpreted in. Defaults to the framework standard unit `ns`.
* `unit_energy`: The units energy depositions read from the input data source should be interpreted in. Defaults to the framework standard unit `MeV`.

### Usage
A example for reading energy depositions from a ROOT file tree named `hitTree`, using only the first five characters of the volume name as detector identifier and meter as unit length, is the following:

```toml
[DepositionReader]
model = "root"
file_name = "g4_energy_deposits.root"
tree_name = "hitTree"
detector_name_chars = 5
unit_length = "m"
```

[@pdg]: http://hepdata.cedar.ac.uk/lbl/2016/reviews/rpp2016-rev-monte-carlo-numbering.pdf
