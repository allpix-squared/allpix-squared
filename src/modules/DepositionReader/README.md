---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DepositionReader"
description: "Energy deposition with information read from a file"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_outputs: ["DepositedCharge", "MCParticle"]
---

## Description

This module allows to read in energy depositions in a sensor volume produced with a different program, e.g. with Geant4 in a standalone simulation of the respective experiment.
The detector geometry for Allpix Squared should resemble the global positions of the detectors of interest in the original simulation.

The assignment of energy deposits to a specific detector in the Allpix Squared simulation is performed using the volume name of the detector element in the original simulation.
Hence, the naming of the detector in the geometry file has to match its name in the input data file.
In order to simplify the aggregation of individual detector element volumes from the original simulation into a single detector, this modules provides the `detector_name_chars` parameter.
It allows matching of the detector name to be performed on a sub-string of the original volume name.

Only energy deposits within a valid volume are considered, i.e. where a matching detector with the same name can be found in the geometry setup.
The global coordinates are then translated to local coordinates of the given detector.
If these are outside the sensor, the energy deposit is discarded and a warning is printed.
The number of electron/hole pairs created by a given energy deposition is calculated using the mean pair creation energy \[[@chargecreation]\], fluctuations are modeled using a Fano factor assuming Gaussian statistics \[[@fano]\].
Default values of both parameters for different sensor materials are included and automatically selected for each of the detectors. A full list of supported materials can be found elsewhere in the manual.
These can be overwritten by specifying the parameters `charge_creation_energy` and `fano_factor` in the configuration.

Track and parent ids of the individual particles which created the energy depositions allow to carry on some of the Monte Carlo particle information from the original simulation.
Monte Carlo particle objects are created for each unique track id, the start and end positions are set to the first and last appearance of the particle, respectively.
A parent id of zero should be used for the primary particle of the simulation, and all track ids have to be recorded before they can be used as parent id.

With the `output_plots` parameter activated, the module produces histograms of the total deposited charge per event for every sensor in units of kilo-electrons.
The scale of the plot axis can be adjusted using the `output_plots_scale` parameter and defaults to a maximum of 100ke.

Currently two data sources are supported, ROOT trees and CSV text files.
Their expected formats are explained in detail in the following.

### ROOT Trees

Data in ROOT trees are interpreted as follows.
The tree with name `tree_name` is opened from the provided ROOT file, and information of energy deposits is read from its individual branches.
By default the expected branch names and types are:

* `event` (integer): Branch for the event number.
* `energy` (double): Branch for the energy deposition information.
* `time` (double): Branch for the time information when the energy deposition took place, calculated from the start of the event.
* `position.x` (double): Leaf of the branch for the `x` position of the energy deposit in global coordinates of the setup.
* `position.y` (double): Leaf of the branch for the `y` position of the energy deposit in global coordinates of the setup.
* `position.z` (double): Leaf of the branch for the `z` position of the energy deposit in global coordinates of the setup.
* `detector` (char array): Branch for the detector or volume name in which the energy was deposited.
* `pdg_code` (integer): Branch for the PDG code particle id if the Monte Carlo particle producing this energy deposition.
* `track_id` (integer): Branch for the track id of the current Monte Carlo particle.
* `parent_id` (integer): Branch for the id of the parent Monte Carlo particle which created the current one.

Entries are read from all branches synchronously and accumulated in the same event until the event id read from the `event` branch changes.

By default, the event numbers need to be sorted with ascending order. This can be disabled by setting `require_sequential_events` to `false`. This is useful when running simulations in mutli-threading mode and merging datasets in the end. Currently only supported in ROOT files.

If the parameters `assign_timestamps` or `create_mcparticles` are set to `false`, no attempt is made in reading the respective branches, independently whether they are present or not.

Different branch names can be configured using the `branch_names` parameter.
It should be noted that new names have to be provided for all branches, i.e. ten names, and that the order of the names has to reflect the order of the branches as listed above to allow for correct assignment.
If `assign_timestamps` or `create_mcparticles` are set to `false`, their branch names (`time` and `track_id`, `parent_id`, respectively) should be omitted from the branch name list.
Individual leafs of branches can be assigned using the dot notation, e.g. `energy.Edep` to access a leaf of the branch `energy` to retrieve the energy deposit information.


### CSV Files

Data in CSV-formatted text files are interpreted as follows.
Empty lines as well as lines starting with a hash (`#`) are ignored, all other lines are interpreted either as event header if they start with `E`, or as energy deposition:

```shell
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

where `<N>` is the current event number, `<PID>` is the PDG particle ID \[[@pdg]\], `<T>` the time of deposition, calculated from the beginning of the event, `<E>` is the deposited energy, `<[X-Z]>` is the position of the energy deposit in global coordinates of the setup, and `<V>` the detector name (volume) the energy deposit should be assigned to.
The values are interpreted in the default framework units unless specified otherwise via the configuration parameters of this module.
`<TRK>` represents the track id of the particle track which has caused this energy deposition, and `<PRT>` the id of the parent particle which created this particle.

If the parameters `assign_timestamps` or `create_mcparticles` are set to `false`, the parsing assumes that the respective columns `<T>` and `<TRK>`, `<PRT>` are not present in the CSV file.

The file should have its end-of-file marker (EOF) in a new line, otherwise the last entry will be ignored.

## Parameters

* `model`: Format of the data file to be read, can either be `csv` or `root`.
* `file_name`: Location of the input data file. The appropriate file extension will be appended if not present, depending on the `model` chosen either `.csv` or `.root`.
* `tree_name`: Name of the input tree to be read from the ROOT file. Only used for the `root` model.
* `branch_names`: List of names of the ten branches to be read from the input ROOT file. Only used for the `root` model. The default names and their content are listed above in the _ROOT Trees_ section.
* `detector_name_chars`: Parameter which allows selecting only a sub-string of the stored volume name as detector name. Could be set to the number of characters from the beginning of the volume name string which should be taken as detector name. E.g. `detector_name_chars = 7` would select `sensor0` from the full volume name `sensor0_px3_14` read from the input file. This is especially useful if the initial simulation in Geant4 has been performed using parameterized volume placements e.g. for individual pixels of a detector. Defaults to `0` which takes the full volume name.
* `charge_creation_energy` : Energy needed to create a charge deposit. Defaults to the energy needed to create an electron-hole pair in the respective sensor material (e.g. 3.64 eV for silicon sensors, \[[@chargecreation]\]). A full list of supported materials can be found elsewhere in the manual.
* `fano_factor`: Fano factor to calculate fluctuations in the number of electron/hole pairs produced by a given energy deposition. Defaults are provided for different sensor materials, e.g. a value of 0.115 for silicon \[[@fano]\]. A full list of supported materials can be found elsewhere in the manual.
* `unit_length`: The units length measurements read from the input data source should be interpreted in. Defaults to the framework standard unit `mm`.
* `unit_time`: The units time measurements read from the input data source should be interpreted in. Defaults to the framework standard unit `ns`.
* `unit_energy`: The units energy depositions read from the input data source should be interpreted in. Defaults to the framework standard unit `MeV`.
* `assign_timestamps`: Boolean to select whether or not time information should be read and assigned to energy deposits. If `false`, all timestamps of deposits are set to 0. Defaults to `true`.
* `create_mcparticles`: Boolean to select whether or not Monte Carlo particle IDs should be read and MCParticle objects created, defaults to `true`.
* `output_plots` : Enables output histograms to be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output plot, defaults to 100ke.

## Usage

An example for reading energy depositions from a ROOT file tree named `hitTree`, using only the first five characters of the volume name as detector identifier and meter as unit length, is the following:

```ini
[DepositionReader]
model = "root"
file_name = "g4_energy_deposits.root"
tree_name = "hitTree"
detector_name_chars = 5
unit_length = "m"
branch_names = ["event", "energy.Edep", "time", "position.x", "position.y", "position.z", "detector", "PDG_code", "track_id", "parent_id"]
```

[@pdg]: http://hepdata.cedar.ac.uk/lbl/2016/reviews/rpp2016-rev-monte-carlo-numbering.pdf
[@chargecreation]: https://doi.org/10.1103/PhysRevB.1.2945
[@fano]: https://doi.org/10.1103%2FPhysRevB.22.5565
