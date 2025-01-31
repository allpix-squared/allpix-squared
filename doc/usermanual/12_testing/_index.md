---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Automated Testing"
description: "The testing framework to prevent regressions."
weight: 12
---

The build system of the framework provides a set of automated tests which are executed by the CI to ensure proper functioning
of the framework and its modules.

{{% alert title="Note" color="info" %}}
Since different Geant4 versions may result in different events, a specific Geant4 version needs to be chosen for the
test suite. All tests in the repository are written for Geant4 11.2 / LCG105.
{{% /alert %}}

The tests can also be manually invoked from the build directory of Allpix Squared with:

```shell
ctest
```

When executed by the CI, the results on passed and failed tests are automatically gathered and prominently displayed in merge
requests along with the overall CI pipeline status. This allows a quick identification of issues without having to manually
search through the log of several CI jobs.

The different subcategories of tests described below can be executed or ignored using the `-E` (exclude) and `-R` (run)
switches of the `ctest` program:

```shell
ctest -R test_performance
```
