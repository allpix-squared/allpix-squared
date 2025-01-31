---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Detector Models"
weight: 3
---

#### I want to use a detector model with one or several small changes, do I have to create a whole new model for this?

No, models can be specialized in the detector configuration file. To specialize a detector model, the key that should be
changed in the standard detector model, e.g. like `sensor_thickness`, should be added as key to the section of the detector
configuration (which already contains the position, orientation and the base model of the detector). Only parameters in the
header of detector models can be changed. If support layers should be changed, or new support layers are needed, a new model
should be created instead. Please refer to [Section 5.2](../05_geometry_detectors/02_models.md) for more
information.
