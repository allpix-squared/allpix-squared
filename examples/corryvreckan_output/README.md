---
# SPDX-FileCopyrightText: 2019-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Corryvreckan Output"
description: "Example for the CorryvreckanWriter module"
---

This example demonstrates how to simulate a full telescope setup and how to store the simulation in a format readable by the Corryvreckan reconstruction framework.

The setup used in this example is the reference simulation published in the Allpix Squared paper \[[@apsq]\].
It consists of six Timepix3 telescope planes \[[@timepix]\] featuring planar silicon sensors with a thickness of 300um.
In addition, another Timepix3 detector is placed as device under test (DUT) between the upstream arm (three planes) and downstream arm (three planes) of the telescope, with a sensor thickness of 50um.
Here, the thickness is directly defined in the geometry file, overwriting the default value from the `timepix` model.
All planes are randomly mis-aligned at the beginning of the simulation using the alignment precision keywords:

```ini
alignment_precision_position = 1mm 1mm 100um
alignment_precision_orientation = 0.2deg 0.2deg 0.2deg
```

The energy deposition module uses Geant4 to replicate the beam conditions found in the CERN SPS North Area beam lines, i.e. a 120GeV Pion beam with a Gaussian width of about 2mm.

The simulation uses different processing paths for the telescope planes and the DUT in order to configure a different electric field, a different granularity for the charge propagation and different settings for the digitization in the front-end.
For this, the `type` and `name` keywords are placed in the configuration file in order to assign the respective modules to specific detectors instantiated in the geometry file.

The results for both the telescope planes and the DUT are written to a ROOT output file in the format of the Corryvreckan reconstruction framework using the CorryvreckanWriter module. Here, two additional keys have to be defined; The detector to be used as reference plane in the reconstruction and the DUT as detector to be excluded from the track fits. More information in these detector roles can be found in the Corryvreckan user manual.

[@apsq]: https://dx.doi.org/10.1016/j.nima.2018.06.020
[@timepix]: http://dx.doi.org/10.1016/j.nima.2007.08.079
