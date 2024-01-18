---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Quick Start"
weight: 4
---

This section serves as a swift introduction to Allpix Squared for users who prefer to start quickly and learn the details
while simulating. The typical user should skip the next paragraphs and continue reading the following sections instead.

Allpix Squared provides a modular, flexible and user-friendly structure for the simulation of independent detectors in
arbitrary configurations. The framework currently relies on the ROOT \[[@root]\] and Boost.Random \[[@boostrandom]\]
libraries, which need to be installed and loaded before using Allpix Squared. For many use cases, installations of Geant4
\[[@geant4]\] and Eigen3 \[[@eigen3]\] are required in addition.

The minimal, default installation can be obtained by executing the commands listed below.

```shell
git clone https://gitlab.cern.ch/allpix-squared/allpix-squared
cd allpix-squared
mkdir build && cd build/
cmake ..
make install
cd ..
```

The binary can then be executed with the provided example configuration file as follows:

```shell
bin/allpix -c examples/example.conf
```

Hereafter, the example configuration can be copied and adjusted to the needs of the user. This example contains a simple
setup of two test detectors. It simulates the whole chain, starting from the passage of the beam, the deposition of charges
in the detectors, the carrier propagation and the conversion of the collected charges to digitized pixel hits. All generated
data is finally stored on disk in ROOT TTrees or other commonly used data formats for later analysis.

After this quick start it is highly recommended to proceed to the other chapters of this user manual. For quickly
resolving common issues, the [Frequently Asked Questions](../13_faq/_index.md) may be particularly useful.


[@root]: http://root.cern.ch/
[@boostrandom]: https://www.boost.org/doc/libs/1_75_0/doc/html/boost_random/reference.html
[@geant4]: https://doi.org/10.1016/S0168-9002(03)01368-8
[@eigen3]: http://eigen.tuxfamily.org
