---
# SPDX-FileCopyrightText: 2019-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "InducedTransfer"
description: "Transfer with induction from nearby charge carriers"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (simon.spannagel@cern.ch)"]
module_inputs: ["PropagatedCharge"]
module_outputs: ["PixelCharge"]
---

## Description
Combines individual sets of propagated charges together to a set of charges on the sensor pixels by calculating the total induced charge during their drift on neighboring pixels by calculating the difference in weighting potential.
This module requires a propagation of both electrons and holes in order to produce sensible results and only works in the presence of a weighting potential.

The induced charge on neighboring pixel implants is defined the Shockley-Ramo theorem \[[@shockley], [@ramo]\] as the difference in weighting potential between the end position $`x_{final}`$ retrieved from the `PropagatedCharge` and the initial position $`x_{initial}`$ of the charge carrier obtained from the `DepositedCharge` object in the history.
The total induced charge is calculated by multiplying the potential difference with the charge of the carrier, viz.

```math
Q_n^{ind} = \int_{t_{initial}}^{t_{final}} I_n^{ind} = q \left( \phi (x_{final}) - \phi(x_{initial}) \right)
```

The resulting induced charge is summed for all propagated charge carriers and returned as a `PixelCharge` object. The number of neighboring pixels taken into account can be configured using the `distance` parameter.

## Parameters
* `distance`: Maximum distance of pixels to be considered for current induction, calculated from the pixel the charge carrier under investigation is below. A distance of `1` for example means that the induced current for the closest pixel plus all neighbors is calculated. It should be noted that the time required for simulating a single event depends almost linearly on the number of pixels the induced charge is calculated for. Usually, for Cartesian sensors a 3x3 grid (9 pixels, distance 1) should suffice since the weighting potential at a distance of more than one pixel pitch often is small enough to be neglected while the simulation time is almost tripled for `distance = 2` (5x5 grid, 25 pixels). To just calculate the induced current in the one pixel the charge carrier is below, `distance = 0` can be used. Defaults to `1`.

## Usage
```ini
[InducedTransfer]
distance = 1
```

[@shockley]: https://doi.org/10.1063/1.1710367
[@ramo]: https://doi.org/10.1109/JRPROC.1939.228757
