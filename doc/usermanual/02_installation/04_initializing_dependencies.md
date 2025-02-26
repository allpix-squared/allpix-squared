---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Initializing the Dependencies"
weight: 4
---

Before continuing with the build, the necessary setup scripts for ROOT and Geant4 (unless a build without Geant4 modules is
attempted) should be executed. In a Bash terminal on a private Linux machine this means executing the following two commands
from their respective installation directories (replacing `<root_install_dir>` with the local ROOT installation directory and
likewise for Geant4):

```shell
source <root_install_dir>/bin/thisroot.sh
source <geant4_install_dir>/bin/geant4.sh
```

On the CERN LXPLUS service, a standard initialization script is available to load all dependencies from the CVMFS
infrastructure. This script should be executed as follows (from the main repository directory):

```shell
source etc/scripts/setup_lxplus.sh
```
