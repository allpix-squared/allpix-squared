---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Objects"
description: "Objects which can be used to transfer data between modules."
weight: 7
---

Allpix Squared provides a set of objects which can be used to transfer data between modules and to store the simulation
results to file. These objects can be read again from file and dispatched to a secondary simulation chain using the
[ROOTObjectReader](../08_modules/rootobjectreader.md) and [ROOTObjectWriter](../08_modules/rootobjectwriter.md) modules which
dispatch them via the messaging system as explained in [Section 4.6](../04_framework/06_messages.md).

Objects stored to a ROOT file can be analyzed using C or Python scripts, the example scripts for both languages described in
[Section 14.3](14_additional/root_analysis_macros.md) are provided in the repository.
