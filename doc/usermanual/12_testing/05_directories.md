---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Directory Variables in Tests"
weight: 5
---

Sometimes it is necessary to pass directories or file names as test input. To facilitate this, the test files can contain
variables which are replaced with the respective paths before being executed. All variable names have to be enclosed in `@`
symbols to be detected and parsed correctly. Variables can be used both in test files and the auxiliary configuration files
such as detector geometry definitions.

The following variables are available:

- `@TEST_DIR@`:
  Directory in which the current test is executed, i.e. where all output files will be placed.

- `@TEST_BASE_DIR@`:
  Base directory under which all tests are being executed. This can be used to reference the output files from another
  test. It should be noted that the respective test has to be referenced using the `#DEPENDS` keyword to ensure that it
  successfully ran before.

- `@PROJECT_SOURCE_DIR@`:
  The root directory of the project. This can for example be used to call a script provided in the `etc/scripts` directory
  of the repository \[[@ap2-repo]\].

The following example demonstrates the use of these variables. A script is called before executing the test and an input file
is expected:

```ini
[Allpix]
detectors_file = "detector.conf"

[DepositionReader]
file_name = "@TEST_DIRECTORY@/deposition.root"

#BEFORE_SCRIPT python @PROJECT_SOURCE_DIR@/etc/scripts/create_deposition_file.py --type a --detector mydetector
```


[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
