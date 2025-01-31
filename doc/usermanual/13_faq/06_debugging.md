---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Debugging"
weight: 6
---

#### What should I include in a bug report?

In all bug reports, the output of `allpix --version` should always be provided as it provides vital information about the
build and the system it is running on.

Ideally, you provide a minimum working example (MWE) of a config that produces the bug. To create an MWE, try to remove as
much possible from your configuration files that does not change the appearance of the bug. This helps developers to
understand the bug more quickly. Please provide all files to reproduce the simulation (main configuration,
geometry configuration and if applicable fields and detector model configuration).

If the bug occurs only in a specific event, use the `skip_events` parameter to fast-forward to this event, fix the random
seed using the `random_seed` parameter and set `number_of_events = 1`. The parameters are explained in
[Section 3.4](../03_getting_started/04_framework_parameters.md).

If the bug is a crash or an unexpected error, please also provide a backtrace (see
"[How do I debug Allpix Squared?](#how-do-i-debug-allpix-squared)").

#### How do I debug Allpix Squared?

A good first step to debugging is to increase the logging level in Allpix Squared. Start by setting the logging level to
`DEBUG` in the module where you expect the bug to happen. For maximum information, you can set the logging level to `TRACE`.
See [Section 3.8](../03_getting_started/08_logging_and_verbosity.md) for details on logging.

If you encounter a bug with Geant4, see "[How can I see the output of Geant4?](#how-can-i-see-the-output-of-geant4)" and
"[How can I enable tracking verbosity for Geant4?](#how-can-i-enable-tracking-verbosity-for-geant4)".

To inspect a crash or an unexpected error in detail, a debugger like [gdb](https://sourceware.org/gdb/) is a useful tool to
find out where exactly the program crashed or why the error was thrown.

Assuming `config.conf` crashes Allpix Squared, a full backtrace is created like this:

```shell
gdb --args allpix -c config.conf
run
thread apply all backtrace full
```

If you want to create a backtrace when Allpix Squared explicitly throws an error, you can use:

```shell
gdb --args allpix -c config.conf
catch throw
run
backtrace
```

#### How can I see the output of Geant4?

Geant4's output stream is configured in the GeometryBuilderGeant4 module. The output stream for Geant4's error stream is
logged with logging level `WARNING`, the standard stream with logging level `TRACE`. These values can be adjusted via
`log_level_g4cerr` and `log_level_g4cout` (see [module documentation](../08_modules/geometrybuildergeant4.md#parameters)).

#### How can I enable tracking verbosity for Geant4?

By setting `geant4_tracking_verbosity = 1` in the DepositionGeant4 module. For details check the parameter entry in the
[module documentation](../08_modules/depositiongeant4.md#parameters). Note that you also need the appropriate logging level
to get the Geant4 output (see "[How can I see the output of Geant4?](#how-can-i-see-the-output-of-geant4)").
