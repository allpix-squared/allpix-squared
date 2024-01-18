---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "ROOTObjectWriter"
description: "Writes simulation objects to a ROOT file"
module_status: "Functional"
module_maintainers: ["Koen Wolters (<koen.wolters@cern.ch>)"]
module_inputs: ["all objects in simulation"]
---

## Description
Reads all messages dispatched by the framework that contain Allpix objects. Every message contains a vector of objects, which is converted to a vector to pointers of the object base class. The first time a new type of object is received, a new tree is created bearing the class name of this object. For every combination of detector and message name, a new branch is created within this tree. A leaf is automatically created for every member of the object. The vector of objects is then written to the file for every event it is dispatched, saving an empty vector if an event does not include the specific object.

If the same type of messages is dispatched multiple times, it is combined and written to the same tree. Thus, the information that they were separate messages is lost. It is also currently not possible to limit the data that is written to file. If only a subset of the objects is needed, the rest of the data should be discarded afterwards.

The event number and the event seed for the random number generator are written to a tree named Event.

In addition to the objects, both the configuration and the geometry setup are written to the ROOT file. The main configuration file is copied directly and all key/value pairs are written to a directory *config* in a subdirectory with the name of the corresponding module. All the detectors are written to a subdirectory with the name of the detector in the top directory *detectors*. Every detector contains the position, rotation matrix and the detector model (with all key/value pairs stored in a similar way as the main configuration).

## Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension `.root` will be appended if not present.
* `include` : Array of object names (without `allpix::` prefix) to write to the ROOT trees, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names (without `allpix::` prefix) that are not written to the ROOT trees (cannot be used together simultaneously with the *include* parameter).

## Usage
To create the default file (with the name *data.root*) containing trees for all objects except for PropagatedCharges, the following configuration can be placed at the end of the main configuration:

```ini
[ROOTObjectWriter]
exclude = "PropagatedCharge"
```

To read back a value of the configuration (here the Allpix Squared version used in the simulation), the following command can be executed on the output file, here named *data.root*:

```bash
root -l data.root -e '(*(string*)_file0->GetDirectory("config/Allpix")->GetKey("version")->ReadObj())'
```
