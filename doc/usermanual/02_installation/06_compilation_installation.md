---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Compilation and Installation"
weight: 6
---

Compiling the framework is now a single command in the build folder created earlier (replacing `<number_of_cores>` with the
number of cores to use for compilation):

```shell
make -j<number_of_cores>
```

The compiled (non-installed) version of the executable can be found at `src/exec/allpix` in the folder. Running Allpix
Squared directly without installing can be useful for developers. It is not recommended for normal users, because the correct
library and model paths are only fully configured during installation.

To install the library to the selected installation location (defaulting to the source directory of the repository) requires
the following command:

```shell
make install
```

The binary is now available as `bin/allpix` in the installation directory. The example configuration files are not installed
as they should only be used as a starting point for your own configuration. They can however be used to check if the
installation was successful. Running the allpix binary with the example configuration using
`bin/allpix -c examples/example.conf` should pass the test without problems if a standard installation is used.
