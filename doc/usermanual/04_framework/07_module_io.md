---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Redirect Module Inputs and Outputs"
weight: 7
---

In the Allpix Squared framework, modules exchange messages typically based on their input and output message types and the
detector type. It is, however, possible to specify a name for the incoming and outgoing messages for every module in the
simulation. Modules will then only receive messages dispatched with the name provided and send named messages to other
modules listening for messages with that specific name. This enables running the same module several times for the same
detector, e.g. to test different parameter settings.

The message output name of a module can be changed by setting the `output` parameter of the module to a unique value. The
output of this module is then not sent to modules without a configured input, because by default modules listens only to data
without a name. The `input` parameter of a particular receiving module should therefore be set to match the value of the
`output` parameter. In addition, it is permitted to set the `input` parameter to the special value `*` to indicate that the
module should listen to all messages irrespective of their name.

An example of a configuration with two different settings for the digitization module is shown below:

```ini
# Digitize the propagated charges with low noise levels
[DefaultDigitizer]
# Specify an output identifier
output = "low_noise"
# Low amount of noise added by the electronics
electronics_noise = 100e
# Default values are used for the other parameters

# Digitize the propagated charges with high noise levels
[DefaultDigitizer]
# Specify an output identifier
output = "high_noise"
# High amount of noise added by the electronics
electronics_noise = 500e
# Default values are used for the other parameters

# Save histogram for 'low_noise' digitized charges
[DetectorHistogrammer]
# Specify input identifier
input = "low_noise"

# Save histogram for 'high_noise' digitized charges
[DetectorHistogrammer]
# Specify input identifier
input = "high_noise"
```
