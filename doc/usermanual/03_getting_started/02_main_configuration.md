---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Main Configuration"
weight: 2
---

The main configuration consists of a set of sections specifying the modules used. All modules are executed in the *linear*
order in which they are defined. There are a few section names which have a special meaning in the main configuration, namely
the following:

- The **global** (framework) header sections:
  These are all zero-length section headers (including the one at the beginning of the file) and all sections marked with
  the header `[Allpix]` (case-insensitive). These are combined and accessed together as the global configuration, which
  contain all parameters of the framework itself (see [Section 3.4](./04_framework_parameters.md) for details). All
  key-value pairs defined in this section are also inherited by all individual configurations as long the key is not
  defined in the module configuration itself.

- The **ignore** header sections:
  All sections with name `[Ignore]` (case-insensitive) are ignored. Key-value pairs defined in the section as well as the
  section itself are discarded by the parser. These section headers are useful for quickly enabling and disabling
  individual modules by replacing their actual name by an ignore section header.

All other section headers are used to instantiate modules of the respective name. Installed modules are loaded automatically.
If problems arise please review the loading rules described in
[Section 4.4](../04_framework/04_modules.md#module-instantiation).

Modules can be specified multiple times in the configuration files, depending on their type and configuration. The type of
the module determines how the module is instantiated:

- If the module is **unique**, it is instantiated only a single time irrespective of the number of detectors. These kinds
  of modules should only appear once in the whole configuration file unless different inputs and outputs are used, as
  explained in [Section 4.7](../04_framework/07_module_io.md).

- If the module is **detector**-specific, it is instantiated once for every detector it is configured to run on. By
  default, an instantiation is created for all detectors defined in the detector configuration file
  (see [Section 3.3](./03_detector_configuration.md), lowest priority) unless one or both of the following parameters are
  specified:

  - `name`:
    An array of detector names the module should be executed for. Replaces all global and type-specific modules of the
    same kind (highest priority).

  - `type`:
    An array of detector types the module should be executed for. Instantiated after considering all detectors specified
    by the name parameter above. Replaces all global modules of the same kind (medium priority).

    Within the same module, the order of the individual instances in the configuration file is irrelevant.

A valid example configuration using the detector configuration above is:

```ini
# Key is part of the empty section and therefore the global configuration
string_value = "example1"
# The location of the detector configuration is a global parameter
detectors_file = "manual_detector.conf"
# The Allpix section is also considered global and merged with the above
[Allpix]
another_random_string = "example2"

# First run a unique module
[MyUniqueModule]
# This module takes no parameters
# [MyUniqueModule] cannot be instantiated another time

# Then run detector modules on different detectors
# First run a module on the detector of type Timepix
[MyDetectorModule]
type = "timepix"
int_value = 1
# Replace the module above for `dut` with a specialized version
# It does not inherit any parameters from earlier modules
[MyDetectorModule]
name = "dut"
int_value = 2
# Run the module on the remaining unspecified detector (`telescope1`)
[MyDetectorModule]
# int_value is not specified, so it uses the default value
```
