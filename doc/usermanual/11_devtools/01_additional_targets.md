---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Additional Targets"
weight: 1
---

A set of testing targets in addition to the standard compilation targets are automatically created by CMake to enable
additional code quality checks and testing. Some of these targets are used by the project's CI, others are intended for
manual checks. Currently, the following targets are provided:

- `make format`:
  Invokes the `clang-format` tool to apply the project's coding style convention to all files of the code base. The format
  is defined in the `.clang-format` file in the root directory of the repository and mostly follows the suggestions defined
  by the standard LLVM style with minor modifications. Most notably are the consistent usage of four whitespace characters
  as indentation and the column limit of 125 characters.

- `make check-format`:
  Also invokes the `clang-format` tool but does not apply the required changes to the code. Instead, it returns an exit
  code 0 (pass) if no changes are necessary and exit code 1 (fail) if changes are to be applied. This is used by the CI.

- `make lint`:
  Invokes the `clang-tidy` tool to provide additional linting of the source code. The tool tries to detect possible errors
  (and thus potential bugs), dangerous constructs (such as uninitialized variables) as well as stylistic errors. In
  addition, it ensures proper usage of modern C++ standards. The configuration used for the `clang-tidy` command can be
  found in the `.clang-tidy` file in the root directory of the repository.

- `make check-lint`:
  Also invokes the `clang-tidy` tool but does not report the issues found while parsing the code. Instead, it returns an
  exit code 0 (pass) if no errors have been produced and exit code 1 (fail) if issues are present. This is used by the CI.

- `make cppcheck`:
  Runs the `cppcheck` command for additional static code analysis. The output is stored in the file `cppcheck_results.xml`
  in XML 2.0 format. It should be noted that some of the issues reported by the tool are to be considered false positives.

- `make cppcheck-html`:
  Compiles a HTML report from the defects list gathered by `make cppcheck`. This target is only available if the
  `cppcheck-htmlreport` executable is found in the `PATH`.

- `make package`:
  Creates a binary release tarball as described in [Section 10.2](./02_packaging.md).
