---
# SPDX-FileCopyrightText: 2019-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "GDMLOutputWriter"
description: "Writes the geometry to a GDML file"
module_status: "Functional"
module_maintainers: ["Koen van den Brandt (<kbrandt@nikhef.nl>)"]
---

## Description

Constructs a GDML output file of the geometry if this module is added.
This feature is to be considered experimental as the GDML implementation of Geant4 is incomplete.

## Dependencies

This module requires an installation `Geant4_GDML`. This option can be enabled by configuring and compiling Geant4 with the option `-DGEANT4_USE_GDML=ON`

## Parameters

* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension .gdml will be appended if not present. Defaults to `Output.gdml`

## Usage

Creating a GDML output file  with the name `myOutputfile.gdml`:

```ini
[GDMLOutputWriter]
file_name = myOutputfile
```
