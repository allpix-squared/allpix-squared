---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Running the testsuite"
weight: 1
---

The tests can also be manually invoked from the build directory of Allpix Squared with:

```sh
ctest
```

When executed by the CI, the results on passed and failed tests are automatically gathered and prominently displayed in merge
requests along with the overall CI pipeline status. This allows a quick identification of issues without having to manually
search through the log of several CI jobs.

The different subcategories of tests described below can be executed or ignored using the `-E` (exclude) and `-R` (run)
switches of the `ctest` program:

```sh
ctest -R test_performance
```
