---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Framework Parameters"
weight: 4
---

The Allpix Squared framework provides a set of global parameters which control and alter its behavior:

- `detectors_file`:
  Location of the file describing the detector configuration (introduced in [Section 3.3](./03_detector_configuration.md)).
  The only *required* global parameter: the framework will fail to start if it is not specified.

- `number_of_events`:
  Determines the total number of events the framework should simulate. Defaults to one (simulating a single event).

- `skip_events`:
  A number of events (and therefore event seeds) to be skipped at start of the run. After skipping, the full
  `number_of_events` will be processed starting from the new event seed. Defaults to zero, i.e. starting with the first
  event seed.

- `root_file`:
  Location relative to the `output_directory` where the ROOT output data of all modules will be written to. The file
  extension `.root` will be appended if not present. Default value is `modules.root`. Directories within the ROOT file will
  be created automatically for all module instantiations.

- `log_level`:
  Specifies the lowest log level which should be reported. Possible values are `FATAL`, `STATUS`, `ERROR`, `WARNING`,
  `INFO`, `DEBUG`, `TRACE` and `PRNG` where all options are case-insensitive. Defaults to the `WARNING` level. More details
  and information about the log levels, including how to change them for a particular module, can be found in
  [Section 3.8](./08_logging_and_verbosity.md). Can be overwritten by the `-v` parameter on the command line (see
  [Section 3.5](./05_allpix_executable.md)).

- `log_format`:
  Determines the log message format to display. Possible options are `SHORT`, `DEFAULT` and `LONG`, where all options are
  case-insensitive. More information can be found in [Section 3.8](./08_logging_and_verbosity.md).

- `log_file`:
  File where the log output should be written to in addition to printing to the standard output (usually the terminal).
  Only writes to standard output if this option is not provided. Another (additional) location to write to can be specified
  on the command line using the `-l` parameter (see [Section 3.5](./05_allpix_executable.md)).

- `output_directory`:
  Directory to write all output files into. Subdirectories are created automatically for all module instantiations. This
  directory will also contain the `root_file` specified via the parameter described above. Defaults to the current working
  directory with the subdirectory `output/` attached.

- `purge_output_directory`:
  Decides whether the content of an already existing output directory is deleted before a new run starts. Defaults to
  `false`, i.e. files are kept but will be overwritten by new files created by the framework.

- `deny_overwrite`:
  Forces the framework to abort the run and throw an exception when attempting to overwrite an existing file. Defaults to
  `false`, i.e. files are overwritten when requested. This setting is inherited by all modules, but can be overwritten in
  the configuration section of each of the modules.

- `random_seed`:
  Seed for the global random seed generator used to initialize seeds for module instantiations. The 64-bit Mersenne Twister
  `mt19937_64` from the C++ Standard Library is used to generate seeds. A random seed from multiple entropy sources will be
  generated if the parameter is not specified. Can be used to reproduce an earlier simulation run.

- `random_seed_core`:
  Optional seed used for pseudo-random number generators in the core components of the framework. If not set explicitly,
  the value `random_seed + 1` is used.

- `library_directories`:
  Additional directories to search for module libraries, before searching the default paths. See
  [Section 4.4](../04_framework/04_modules.md#module-instantiation) for more information.

- `model_paths`:
  Additional files or directories from which detector models should be read besides the standard search locations.

- `performance_plots`:
  Enable the creation of performance plots showing the processing time required per event both for individual modules and
  the full module stack. Defaults to `false`.

- `multithreading`:
  Enable multithreading for the framework. Defaults to `true`. More information about multithreading can be found in
  [Section 4.3](../04_framework/04_modules.md#multithreading-parallel-execution-of-events).

- `workers`:
  Specify the number of workers to use in total, should be strictly larger than zero. Only used if `multithreading` is set
  to `true`. Defaults to the number of native threads available on the system minus one, if this can be determined,
  otherwise one thread is used.

- `buffer_per_worker`:
  Specify the buffer depth available per worker for buffered modules to cache partially processed events until execution in
  the correct order can be guaranteed (see [Section 4.10](../04_framework/10_multithreading.md)). Defaults to `256`.
