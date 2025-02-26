---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Error Reporting and Exceptions"
weight: 9
---

Allpix Squared generally follows the principle of throwing exceptions in all cases where something is definitely wrong.
Exceptions are also thrown to signal errors in the user configuration. It does not attempt to circumvent problems or correct
configuration mistakes, and the use of error return codes is to be discouraged. The asset of this method is that errors
cannot easily be ignored and the code is more predictable in general.

For warnings and information messages, the logging system should be used extensively. This helps both in following the
progress of the simulation and in debugging problems. Care should however be taken to limit the amount of messages in levels
higher than `DEBUG` or `TRACE`. More details about the logging levels and their usage can be found in
[Section 3.8](../03_getting_started/08_logging_and_verbosity.md).

The base exceptions in Allpix Squared are available via the utilities. The most important exception base classes are the
following:

- `ConfigurationError`:
  All errors related to incorrect user configuration. This could indicate a non-existing configuration file, a missing key
  or an invalid parameter value.

- `RuntimeError`:
  All other errors arising at run-time. Could be related to incorrect configuration if messages are not correctly passed or
  non-existing detectors are specified. Could also be raised if errors arise while loading a library or executing a module.

- `LogicError`:
  Problems related to modules which do not properly follow the specifications, for example if a detector module fails to
  pass the detector to the constructor. These methods should never be raised for correctly implemented modules and should
  therefore not be of any concern for the end users. Reporting this type of error can help developers during the
  development of new modules.

There are only four exceptions that are supposed to be used in specific modules, outside of the core framework. These
exceptions should be used to indicate errors that modules cannot handle themselves:

- `InvalidValueError`:
  Derived from configuration exceptions. Signals any problem with the value of a configuration parameter not related to
  parsing or conversion to the required type. Can for example be used for parameters where the possible valid values are
  limited, like the set of logging levels, or for paths that do not exist. An example is shown below:

  ```cpp
  void run(Event* event) {
      // Fetch a key from the configuration
      std::string value = config.get("key");

      // Check if it is a 'valid' value
      if(value != 'A' && value != "B") {
          // Raise an error if it the value is not valid
          //   provide the configuration object, key and an explanation
          throw InvalidValueError(config, "key", "A and B are the only allowed values");
      }
  }
  ```

- `InvalidCombinationError`:
  Derived from configuration exceptions. Signals any problem with a combination of configuration parameters used. This
  could be used if several optional but mutually exclusive parameters are present in a module, and it should be ensured
  that only one is specified at the time. The exceptions accepts the list of keys as initializer list. An example is shown
  below:

  ```cpp
  void run(Event* event) {
      // Check if we have mutually exclusive options defined:
      if(config.count({"exclusive_opt_a", "exclusive_opt_b"}) > 1) {
          // Raise an error if the combination of keys is not valid
          //   provide the configuration object, keys and an explanation
          throw InvalidCombinationError(config, {"exclusive_opt_a", "exclusive_opt_b"},
              "Options A and B are mutually exclusive, specify only one.");
      }
  }
  ```

- `ModuleError`:
  Derived from module exceptions. Should be used to indicate any runtime error in a module not directly caused by an
  invalid configuration value, for example that it is not possible to write an output file. A reason should be given to
  indicate what the source of problem is.

- `EndOfRunException`:
  Derived from module exceptions. Should be used to request the end of event processing in the current run, e.g. if a
  module reading in data from a file reached the end of its input data.
