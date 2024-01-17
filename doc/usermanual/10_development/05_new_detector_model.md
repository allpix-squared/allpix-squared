---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Adding a New Detector Model"
weight: 5
---

Custom detector models based on the detector classes provided with Allpix Squared can easily be added to the framework. In
particular [Section 5.2](../05_geometry_detectors/02_models.md) explains all parameters of the
detector models currently available. The default models provided in the `models` directory of the repository can serve as
examples. To create a new detector model, the following steps should be taken:

1. Create a new file with the name of the model followed by the `.conf` suffix (for example `your_model.conf`).

2. Add a configuration parameter `type` with the type of the model, at the moment either `monolithic` or `hybrid` for
   respectively monolithic sensors or hybrid models with bump bonds and a separate readout chip.

3. Add all required parameters and possibly optional parameters as explained in
   [Section 5.2](../05_geometry_detectors/02_models.md).

4. Include the detector model in the search path of the framework by adding the `model_paths` parameter to the general
   setting of the main configuration (see [Section 3.4](../03_getting_started/04_framework_parameters.md)), pointing either
   directly to the detector model file or the directory containing it. It should be noted that files in this path will
   overwrite models with the same name in the default model folder.

Models should be contributed to the main repository to make them available to other users of the framework. To add the
detector model to the framework the configuration file should be moved to the folder `models` of the repository. The file
should then be added to the installation target in the `CMakeLists.txt` file of the `models` directory. Afterwards, a merge
request can be created via the mechanism provided by the software repository \[[@ap2-repo]\].


[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
