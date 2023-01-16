---
# SPDX-FileCopyrightText: 2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Releases on CVMFS"
weight: 8
---

For each release a binary tarball is created, which is uploaded to Allpix Squared's [binary releases page](@ap2-bin-releases)
and published to CERN's VM file system (CVMFS) \[[@cvmfs]\]. They can be used to run Allpix Squared without building it
beforehand. Compared to the Docker images mentioned in [Section 2.7](./07_docker_images.md), which can run on any operating
system, binary releases are tied to a specific operating system.

Binaries for Allpix Squared are currently provided for CentOS 7 (both in a GCC and LLVM variant), CentOS 8 (GCC) and MacOS.
Details on the deployment process are given in [Setcion 11.4](../11_devtools/04_deployment.md).

## Installation via CVMFS

If CVMFS \[[@cvmfs]\] is available on the system, Allpix Squared can be loaded via

```shell
source /cvmfs/clicdp.cern.ch/software/allpix-squared/<version>/<system-specifier>/setup.sh
```

Where `<version>` should be replaced with the desired Allpix Squared version (e.g. `2.4.0`) and `<system-specifier>` with the
specifier for the system CVMFS is running on (e.g. `x86_64-centos7-gcc11-opt`).

To verify if Allpix Squared is working, you can run `allpix --version`.

## Installation via binary tarball

If CVMFS is not available on the system, binary tarballs of each release for different systems can be downloaded from Allpix
Squared's [binary releases page](@ap2-bin-releases). The tarball contains a `setup.sh` script that needs to be sourced.

Example for Allpix Squared version 2.4.0 running CentOS 7:

```shell
wget https://project-allpix-squared.web.cern.ch/releases/allpix-squared-2.4.0-x86_64-centos7-gcc11-opt.tar.gz
tar -xf allpix-squared-2.4.0-x86_64-centos7-gcc11-opt.tar.gz
source allpix-squared-2.4.0-x86_64-centos7-gcc11-opt/setup.sh
allpix --version
```

TODO: actually no - requires CVMFS -> drop since we deploy to CVMFS anyway?


[@ap2-bin-releases]: https://project-allpix-squared.web.cern.ch/releases/
[@cvmfs]: https://pos.sissa.it/070/052/
