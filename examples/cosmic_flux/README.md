---
# SPDX-FileCopyrightText: 2021-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Cosmic Flux"
description: "Simulation of cosmic muon flux using the DepositionCosmics module"
---

This example illustrates how the `DepositionCosmics` module is used to model the flux of cosmic muons in Allpix Squared. Python analysis code is included to calculate the flux through the detector from the MCParticle objects.

## Usage
First change into the example directory. Run the simulation example from here:
```shell
allpix -c cosmic_flux.conf
```
To analyze the tracks of the MCParticles, issue
```shell
python analysis/analysis.py -l PATH_TO_ALLPIX_INSTALL/lib/libAllpixObjects.so
```
The library flag is only required when your `allpix-squared/lib` path is not in `LD_LIBRARY_PATH`.
Notice that the analysis script relies on the python modules `uncertainties`, `tqdm`, `matplotlib` and `numpy`.
