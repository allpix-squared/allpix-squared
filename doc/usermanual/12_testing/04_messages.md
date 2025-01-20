---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Warning and Error Messages During Testing"
weight: 4
---

If no explicit fail conditions are specified, the test will fail if any `WARNING`, `ERROR` or `FATAL` appears in the output
log unless it is already part of the pass condition. For example, if a test is supposed to pass in case of an error provoked

```shell
(FATAL) [I:GeometryBuilderGeant4] Error during execution of run:
                                  Could not find a detector model of type 'missing_model'
                                  Please check your configuration and modules. Cannot continue.
```

The full error message including the `FATAL` has to be provided as pass condition:

```ini
#PASS (FATAL) [I:GeometryBuilderGeant4] Error during execution of run:\nCould not find a detector model of type 'missing_model'
```

If a test is expected to create multiple error or warning messages which cannot be matched with a single pass condition, the
`#FAIL` parameter should be set explicitly to avoid matching the respective flags:

```ini
# This test created multiple WARNING messages, we exclude WARNING from the
# fail expression by explicitly defining it as FATAL only:
#PASS (ERROR) Multithreading disabled since the current module configuration does not support it
#FAIL FATAL
```
