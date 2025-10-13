---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Logging and other Utilities"
weight: 8
---

The Allpix Squared framework provides a set of utilities which improve the usability of the framework and extend the
functionality provided by the C++ Standard Template Library (STL). The former includes a flexible and easy-to-use logging
system and an easy-to-use framework for units that supports converting arbitrary combinations of units to common base units
which can be used transparently throughout the framework. The latter comprise tools which provide functionality the C++20
standard does not contain. These utilities are used internally in the framework and are only shortly discussed.

### Logging system

The logging system is built to handle input/output in the same way as `std::cin` and `std::cout` do. This approach is both
flexible and easy to read. The system is globally configured, thus only one logger instance exists. The following
commands are available for sending messages to the logging system at a level of `LEVEL`:

* `LOG(LEVEL)`:
  Sends a message with severity level `LOG(LEVEL)` to the logging system. Example:

  ```cpp
      LOG(LEVEL) << "this is an example message with an integer and a double " << 1 << 2.0;
  ```

  A new line and carriage return is added at the end of every log message. Multi-line log messages can be used by adding
  new line commands to the stream. The logging system will automatically align every new line under the previous message
  and will leave the header space empty on new lines.

* `LOG_ONCE(LEVEL)`:
  Same as `LOG()`, but will only log this message once over the full run, even if the logging function is called multiple
  times. Example:

  ```cpp
      LOG_ONCE(INFO) << "This message will appear once only, even if present in every event...";
  ```

  This can be used to log warnings or messages e.g. from the `run()` function of a module without flooding the log output
  with the same message for every event. The message is preceded by the information that further messages will be
  suppressed.

* `LOG_N(LEVEL, NUMBER)`:
  Same as `LOG_ONCE()` but allows to specify the number of times the message will be logged via the additional parameter
  `NUMBER`. Example:

  ```cpp
      LOG_N(INFO, 10) << "This message will appear maximally 10 times throughout the run.";
  ```

  The last message is preceded by the information that further messages will be suppressed.

* `LOG_PROGRESS(LEVEL, IDENTIFIER)`:
  This function allows to update the message to be updated on the same line for simple progressbar-like functionality.
  Example:

  ```cpp
      LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Running event " << n << " of " << number_of_events;
  ```

  Here, the `IDENTIFIER` is a unique string identifying this output stream in order not to mix different progress reports.

If the output is a terminal screen the logging output will be coloured to make it easier to identify warnings and error
messages. This is disabled automatically for all non-terminal outputs.

More details about the logging levels and formats can be found in
[Section 3.8](../03_getting_started/08_logging_and_verbosity.md).

### Unit system

Correctly handling units and conversions is of paramount importance. Having a separate C++ type for every unit would however
be too cumbersome for a lot of operations, therefore units are stored in standard C++ floating point types in a default unit
which all code in the framework should use for calculations. In configuration files, as well as for logging, it is however
useful to provide quantities in different units.

The unit system allows adding, retrieving, converting and displaying units. It is a global system transparently used
throughout the framework. Examples of using the unit system are given below:

```cpp
// Define the standard length unit and an auxiliary unit
Units::add("mm", 1);
Units::add("m", 1e3);
// Define the standard time unit
Units::add("ns", 1);
// Get the units given in m/ns in the defined framework unit (mm/ns)
Units::get(1, "m/ns");
// Get the framework unit (mm/ns) in m/ns
Units::convert(1, "m/ns");
// Return the unit in the best type (lowest number larger than one) as string.
// The input is in default units 2000mm/ns and the 'best' output is 2m/ns (string)
Units::display(2e3, {"mm/ns", "m/ns"});
```

A description of the use of units in config files within Allpix Squared was presented in
[Section 3.1](../03_getting_started/01_configuration_files.md#parsing-types-and-units).

### Internal utilities

STL only provides string conversions for standard types using `std::stringstream` and `std::to_string`, which do not allow
parsing strings encapsulated in pairs of double quote (`"`) characters nor integrating different units. Furthermore it does
not provide wide flexibility to add custom conversions for other external types in either way.

The framework's `to_string` and `from_string` methods provided by its **string utilities** do allow for these flexible
conversions, and are extensively used in the configuration system. Conversions of numeric types with a unit attached are
automatically resolved using the unit system discussed above. The string utilities also include trim and split strings
functions missing in the STL.

Furthermore, the Allpix Squared tool system contains extensions to allow automatic conversions for ROOT and Geant4 types as
explained in [Section 14.1](../14_additional/01_tools.md#root-and-geant4-utilities).

To be able to provide cross-platform reproducibility of simulations, Allpix Squared uses random number distributions from the
Boost.Random library. Their implementation is fixed and therefore does not depend on the standard library of the respective
platform. For ease of use, the most common ones are exported to the `allpix::` namespace.

The pseudo-random number generator used for event seeds and random number generation within modules is the `std::mt19937_64`,
a 64-bit Mersenne Twister algorithm. In order to allow for debugging of the random number distribution in a multithreaded
environment, Allpix Squared provides the `allpix::RandomNumberGenerator` wrapper around the STL object, which allows to
print every random number drawn from the generator to the logging facilities when setting the log level to `PRNG`.
