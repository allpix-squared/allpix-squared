---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "The allpix Executable"
weight: 5
---

The `allpix` executable functions as the interface between the user and the framework. It is primarily used to provide the
main configuration file, but also allows to add and overwrite options from the main configuration file. This is both useful
for quick testing as well as for batch processing of simulations.

The executable handles the following arguments:

- `-c <file>`:
  Specifies the configuration file to be used for the simulation, relative to the current directory. This is the only
  *required* argument, the simulation will fail to start if this argument is not given.

- `-l <file>`:
  Specify an additional location to forward log output to, besides standard output and the location specified in the
  framework parameters explained in [Section 3.4](./04_framework_parameters.md).

- `-v <level>`:
  Sets the global log verbosity level, overwriting the value specified in the configuration file described in
  [Section 3.4](./04_framework_parameters.md). Possible values are `FATAL`, `STATUS`, `ERROR`, `WARNING`, `INFO` and
  `DEBUG`, `TRACE` and `PRNG` where all options are case-insensitive. The module specific logging level introduced in
  [Section 3.8](./08_logging_and_verbosity.md) is not overwritten.

- `-j <workers>`:
  Enables multithreaded event processing with the given number of worker threads. This is equivalent to passing the
  framework parameters `-o multithreading=true -o workers=<workers>` to the executable.

- `--version`:
  Prints the version and build time of the executable and terminates the program.

- `-o <option>`:
  Passes extra framework or module options which are added and overwritten in the main configuration file. This argument
  may be specified multiple times, to add multiple options. Options are specified as key/value pairs in the same syntax as
  used in the configuration files (refer to [Section 4.2](../04_framework/03_configuration.md#file-format) for more
  details), but the key is extended to include a reference to a configuration section or instantiation in shorthand
  notation. There are three types of keys that can be specified:

  - Keys to set **framework parameters**:
    These have to be provided in exactly the same way as they would be in the main configuration file (a section does not
    need to be specified). An example to overwrite the standard output directory would be
    `allpix -c <file> -o output_directory="run123456"`.

  - Keys for **module configurations**:
    These are specified by adding a dot (`.`) between the module and the actual key as it would be given in the
    configuration file (thus `module.key`). An example to overwrite the deposited particle to a positron would be
    `allpix -c <file> -o DepositionGeant4.particle_type="e+"`.

  - Keys to specify values for a particular **module instantiation**:
    The identifier of the instantiation and the name of the actual key are split by a dot (`.`), in the same way as for
    keys for module configurations (thus `identifier.key`). The unique identifier for a module can contains one or more
    colons (`:`) to distinguish between various instantiations of the same module. The exact name of an identifier
    depends on the name of the detector and the optional input and output name. Those identifiers can be extracted from
    the logging section headers. An example to change the temperature of propagation for a particular instantiation for a
    detector named `dut` could be `allpix -c <file> -o GenericPropagation:dut.temperature=273K`.

  Note that only the single argument directly following the `-o` is interpreted as the option. If there is whitespace in
  the key/value pair this should be properly enclosed in quotation marks to ensure the argument is parsed correctly.

- `-g <option>`:
  Passes extra detector options which are added and overwritten in the detector configuration file. This argument can be
  specified multiple times, to add multiple options. The options are parsed in the same way as described above for module
  options, but only one type of key can be specified to overwrite an option for a single detector. These are specified by
  adding a dot (`.`) between the detector and the actual key as it would be given in the detector configuration file (thus
  `detector.key`). This method also works for customizing detector models as described in
  [Section 5.2](../05_geometry_detectors/02_models.md). An example to overwrite the sensor thickness for
  a particular detector named `detector1` to `50um` would be `allpix -c <file> -g detector1.sensor_thickness=50um`.

No interaction with the framework is possible during the simulation. Signals can however be send using keyboard shortcuts to
terminate the simulation, either gracefully or with force. The executable understand the following signals:

- SIGINT (`CTRL+C`):
  Request a graceful shutdown of the simulation. This means the currently processed events are finished, while events
  placed on the buffer as well as all additionally requested events from the configuration file are ignored. After
  finishing the current events, the finalization stage is run for every module to ensure that the simulation terminates
  properly. This signal can be useful when too many events are specified and the simulation takes too long to finish
  entirely, but the output generated so far should still be kept.

- SIGTERM:
  Same as SIGINT, request a graceful shutdown of the simulation. This signal is emitted e.g. by the `kill` command or by
  cluster computing schedulers to ask for a termination of the job.

- SIGQUIT (`CTRL+\`):
  Forcefully terminates the simulation. It is not recommended to use this signal as it will normally lead to the loss of
  all generated data. This signal should only be used when graceful termination is for any reason not possible.
