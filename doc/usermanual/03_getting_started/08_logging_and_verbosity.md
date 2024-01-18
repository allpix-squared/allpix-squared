---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Logging and Verbosity Levels"
weight: 8
---

Allpix Squared is designed to identify mistakes and implementation errors as early as possible and to provide the user with
clear indications about the problem. The amount of feedback can be controlled using different log levels which are inclusive,
i.e. lower levels also include messages from all higher levels. The global log level can be set using the global parameter
`log_level`. The log level can be overridden for a specific module by adding the `log_level` parameter to the respective
configuration section. The following log levels are supported:

- **FATAL**:
  Indicates a fatal error that will lead to direct termination of the application. Typically only emitted in the main
  executable after catching exceptions as they are the preferred way of fatal error handling (as discussed in
  [Section 4.9](../04_framework/09_error_reporting.md)). An example of a fatal error is an invalid configuration parameter.

- **STATUS**:
  Important information about the status of the simulation. Is only used for messages which have to be logged in every run
  such as the global seed for pseudo-random number generators and the current progress of the run.

- **ERROR**:
  Severe error that should not occur during a normal well-configured simulation run. Frequently leads to a fatal error and
  can be used to provide extra information that may help in finding the problem (for example used to indicate the reason a
  dynamic library cannot be loaded).

- **WARNING**:
  Indicate conditions that should not occur normally and possibly lead to unexpected results. The framework will however
  continue without problems after a warning. A warning is for example issued to indicate that an output message is not used
  and that a module may therefore perform unnecessary work.

- **INFO**:
  Information messages about the physics process of the simulation. Contains summaries of the simulation details for every
  event and for the overall simulation. Should typically produce maximum one line of output per event and module.

- **DEBUG**:
  In-depth details about the progress of the simulation and all physics details of the simulation. Produces large volumes
  of output per event, and should therefore only be used for debugging the physics simulation of the modules.

- **TRACE**:
  Messages to trace what the framework or a module is currently doing. Unlike the DEBUG level, it does not contain any
  direct information about the physics of the simulation but rather indicates which part of the module or framework is
  currently running. Mostly used for software debugging or determining performance bottlenecks in the simulations.

- **PRNG**:
  This level enables printing of every single pseudo-random number requested from any generator used in the framework. This
  can be useful in order to investigate random number distribution among threads and events.

{{% alert title="Warning" color="warning" %}}
It is not recommended to set the `log_level` higher than **WARNING** in a typical simulation as important messages may be
missed. Setting too low logging levels should also be avoided since printing many log messages will significantly slow down
the simulation.
{{% /alert %}}

The logging system supports several formats for displaying the log messages. The following formats are supported via the
global parameter `log_format` or the individual module parameter with the same name:

- **SHORT**:
  Displays the data in a short form. Includes only the first character of the log level followed by the configuration
  section header and the message.

- **DEFAULT**:
  The default format. Displays system time, log level, section header and the message itself.

- **LONG**:
  Detailed logging format. Displays all of the above but also indicates source code file and line where the log message was
  produced. This can help in debugging modules.

More details about the logging system and the procedure for reporting errors in the code can be found in
[Section 4.8](../04_framework/08_logging.md#logging-system) and [Section 4.9](../04_framework/09_error_reporting.md).
