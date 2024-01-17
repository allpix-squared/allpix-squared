---
# SPDX-FileCopyrightText: 2018-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "TextWriter"
description: "Writes simulation objects to a text file"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_inputs: ["all objects in simulation"]
---

## Description
This module allows to write any object from the simulation to a plain ASCII text file. It reads all messages dispatched by the framework containing Allpix objects. The data content of each message is printed into the text file, while events are separated by an event header:

```
=== <event number> ===
```

and individual detectors by the detector marker:

```
--- <detector name> ---
```

The `include` and `exclude` parameters can be used to restrict the objects written to file to a certain type.

## Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension `.txt` will be appended if not present.
* `include` : Array of object names (without `allpix::` prefix) to write to the ASCII text file, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names (without `allpix::` prefix) that are not written to the ASCII text file (cannot be used together simultaneously with the *include* parameter).

## Usage
To create the default file (with the name *data.txt*) containing entries only for PixelHit objects, the following configuration can be placed at the end of the main configuration:

```ini
[TextWriter]
include = "PixelHit"
```
