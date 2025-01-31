---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Storing Output Data"
weight: 9
---

Storing the simulation output to persistent storage is of primary importance for subsequent reprocessing and analysis. Allpix
Squared primarily uses ROOT for storing output data, given that it is a standard tool in High-Energy Physics and allows
objects to be written directly to disk. The `ROOTObjectWriter` automatically saves all objects created in a `TTree`
\[[@roottree]\]. It stores separate trees for all object types and creates branches for every unique message name: a
combination of the detector, the module and the message output name as described in
[Section 4.7](../04_framework/07_module_io.md). For each event, values are added to the leaves of the branches containing the
data of the objects. This allows for easy histogramming of the acquired data over the total run using standard ROOT
utilities.

Relations between objects within a single event are internally stored as ROOT TRefs \[[@roottref]\], allowing retrieval of
related objects as long as these are loaded in memory. An exception will be thrown when trying to access an object which is
not in memory. Refer to [Section 7.2](../07_objects/02_object_history.md) for more information about object history.

In order to save all objects of the simulation, a `ROOTObjectWriter` module has to be added with a `file_name` parameter to
specify the file location of the created ROOT file in the global output directory. The file extension `.root` will be
appended if not present. The default file name is `data`, i.e. the file `data.root` is created in the output directory. To
replicate the default behaviour the following configuration can be used:

```ini
# The object writer listens to all output data
[ROOTObjectWriter]
# specify the output file (default file name is used if omitted)
file_name = "data"
```

The generated output file can be analyzed using ROOT macros. A simple macro for converting the results to a tree with
standard branches for comparison is described in
[Section 14.3](../14_additional/root_analysis_macros.md#display-monte-carlo-hits-python).

It is also possible to read object data back in, in order to dispatch them as messages to further modules. This feature is
intended to allow splitting the execution of parts of the simulation into independent steps, which can be repeated multiple
times. The tree data can be read using a `ROOTObjectReader` module, which automatically dispatches all objects to the correct
module instances. An example configuration for using this module is:

```ini
# The object reader dispatches all objects in the tree
[ROOTObjectReader]
# path to the output data file, absolute or relative to the configuration file
file_name = "../output/data.root"
```

The Allpix Squared framework comes with a few more output modules which allow data storage in different formats, such as the
[`LCIOWriter`](../08_modules/lciowriter.md) for the LCIO persistency event data model \[[@lcio]\], the
[`RCEWriter`](../08_modules/rcewriter.md) for the native RCE file format \[[@rce]\], or the
[`CorryvreckanWriter`](../08_modules/corryvreckanwriter.md) for the Corryvreckan reconstruction framework data format.
Consult [Chapter 8](../08_modules/_index.md) for all output modules.


[@roottree]: https://root.cern.ch/root/htmldoc/guides/users-guide/Trees.html
[@roottref]: https://root.cern.ch/root/htmldoc/guides/users-guide/InputOutput.html
[@lcio]: https://doi.org/10.1109/NSSMIC.2012.6551478
[@rce]: https://twiki.cern.ch/twiki/bin/view/Atlas/RCEDevelopmentLab
