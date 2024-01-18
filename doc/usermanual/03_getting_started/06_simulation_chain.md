---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Setting up the Simulation Chain"
weight: 6
---

In the following, the framework parameters are used to set up a fully functional simulation. Module parameters are shortly
introduced when they are first used. For more details about the module parameters, the respective module documentation in
[Chapter 8](../08_modules/_index.md) should be consulted. A typical simulation in Allpix Squared will contain the following
components:

- The **geometry builder**, responsible for creating the external Geant4 geometry from the internal geometry. In this
  document, *internal geometry* refers to the detector parameters used by Allpix Squared for coordinate transformations and
  conversions throughout the simulation, while *external geometry* refers to the constructed Geant4 geometry used for
  charge carrier deposition (and possibly visualization).

- The **deposition** module that simulates the particle beam creating charge carriers in the detectors using the provided
  physics list (containing a description of the simulated interactions) and the geometry created above.

- A **propagation** module that propagates the charges through the sensor.

- A **transfer** module that transfers the charges from the sensor electrodes and assigns them to a pixel of the readout
  electronics.

- A **digitizer** module which converts the charges in the pixel to a detector hit, simulating the front-end electronics
  response.

- An **output** module, saving the data of the simulation. The Allpix Squared standard file format is a ROOT `TTree`, which
  is described in detail in [Section 4.7](./09_storing_output_data.md).

In this example, charge carriers will be deposited in the three sensors defined in the detector configuration file from
[Section 3.3](./03_detector_configuration.md). All charge carriers deposited in the different sensors will be propagated and
digitized. Finally, monitoring histograms for the device under test (DUT) will be recorded in the framework's main ROOT file
and all simulated objects, including the entry and exit positions of the simulated particles (Monte Carlo truth), will be
stored in a ROOT file using the Allpix Squared format. An example configuration file implementing this would look like:

```ini
# Global configuration
[Allpix]
# Simulate a total of 5 events
number_of_events = 5
# Use the short logging format
log_format = "SHORT"
# Location of the detector configuration
detectors_file = "manual_detector.conf"

# Read and instantiate the detectors and construct the Geant4 geometry
[GeometryBuilderGeant4]

# Initialize physics list and particle source
[DepositionGeant4]
# Use a Geant4 physics lists with EMPhysicsStandard_option3 enabled
physics_list = FTFP_BERT_LIV
# Use a charged pion as particle
particle_type = "pi+"
# Set the energy of the particle
source_energy = 120GeV
# Origin of the beam
source_position = 0 0 -12mm
# The direction of the beam
beam_direction = 0 0 1
# Use a single particle in a single event
number_of_particles = 1

# Propagate the charge carriers through the sensor
[GenericPropagation]
# Set the temperature of the sensor
temperature = 293K
# Propagate multiple charges at once
charge_per_step = 50

# Transfer the propagated charges to the pixels
[SimpleTransfer]
max_depth_distance = 5um

# Digitize the propagated charges
[DefaultDigitizer]
# Noise added by the readout electronics
electronics_noise = 110e
# Threshold for a hit to be detected
threshold = 600e
# Threshold dispersion
threshold_smearing = 30e
# Noise added by the digitisation
qdc_smearing = 100e

# Save histograms to the ROOT output file
[DetectorHistogrammer]
# Save histograms for the "dut" detector only
name = "dut"

# Store all simulated objects to a ROOT file with TTrees
[ROOTObjectWriter]
# File name of the output file
file_name = "allpix-squared-output"
# Ignore initially deposited charges and propagated carriers:
exclude = DepositedCharge, PropagatedCharge
```

This configuration is available in the repository \[[@ap2-repo]\] at `etc/manual.conf`. The detector configuration file can
be found at `etc/manual_detector.conf`.

The simulation is started by passing the path of the main configuration file to the `allpix` executable as follows:

```shell
allpix -c etc/manual.conf
```

The detector histograms such as the hit map are stored in the ROOT file `output/modules.root` in the TDirectory
`DetectorHistogrammer/`.

If problems occur when exercising this example, it should be made sure that an up-to-date and properly installed version of
Allpix Squared is used (see the installation instructions in [Chapter 2](../02_installation/_index.md)). If modules or models
fail to load, more information about potential issues with the library loading can be found in the detailed framework
description in [Section 4.4](../04_framework/04_modules.md#module-instantiation).


[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
