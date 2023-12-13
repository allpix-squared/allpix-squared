---
# SPDX-FileCopyrightText: 2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Releases on CVMFS"
weight: 8
---

For each release a binary tarball is created, which is published to CERN's VM file system (CVMFS) \[[@cvmfs]\]. They can be
used to run Allpix Squared without building it beforehand. Compared to the Docker images mentioned in
[Section 2.7](./07_docker_images.md), which can run on any operating system, binary releases are tied to a specific operating
system.

Binaries for Allpix Squared are currently provided for CentOS 7 (GCC only), AlmaLinux 9/Red Hat Enterprise Linux 9/EL9 (both in a GCC and LLVM variant) and MacOS.
Details on the deployment process are given in [Setcion 11.4](../11_devtools/04_deployment.md).

To use Allpix Squared from CVMFS, run:

```shell
source /cvmfs/clicdp.cern.ch/software/allpix-squared/<version>/<system-specifier>/setup.sh
```

Where `<version>` should be replaced with the desired Allpix Squared version (e.g. `2.4.0`) and `<system-specifier>` with the
specifier for the system CVMFS is running on (e.g. `x86_64-centos7-gcc11-opt`).

To verify if Allpix Squared is working, you can run `allpix --version`.


[@cvmfs]: https://pos.sissa.it/070/052/
