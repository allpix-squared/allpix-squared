<!--
SPDX-FileCopyrightText: 2019-2022 CERN and the Allpix Squared authors
SPDX-License-Identifier: CC-BY-4.0
-->

# InducedTransfer
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Status**: Functional  
**Input**: PropagatedCharge  
**Output**: PixelCharge

### Description
Combines individual sets of propagated charges together to a set of charges on the sensor pixels by calculating the total induced charge during their drift on neighboring pixels by calculating the difference in weighting potential.
This module requires a propagation of both electrons and holes in order to produce sensible results and only works in the presence of a weighting potential.

The induced charge on neighboring pixel implants is defined the Shockley-Ramo theorem [@shockley] [@ramo] as the difference in weighting potential between the end position $`x_{final}`$ retrieved from the `PropagatedCharge` and the initial position $`x_{initial}`$ of the charge carrier obtained from the `DepositedCharge` object in the history.
The total induced charge is calculated by multiplying the potential difference with the charge of the carrier, viz.

$` Q_n^{ind}  = \int_{t_{initial}}^{t_{final}} I_n^{ind} = q \left( \phi (x_{final}) - \phi(x_{initial}) \right)`$

The resulting induced charge is summed for all propagated charge carriers and returned as a `PixelCharge` object. The number of neighboring pixels taken into account can be configured using the `induction_matrix` parameter.

### Parameters
* `induction_matrix`: Size of the pixel sub-matrix for which the induced charge is calculated, provided as number of pixels in x and y. The numbers have to be odd and default to `3, 3`. Usually, a 3x3 grid (9 pixels) should suffice since the weighting potential at a distance of more than one pixel pitch normally is small enough to be neglected.

### Usage
```toml
[InducedTransfer]
induction_matrix = 3 3
```

[@shockley]: https://doi.org/10.1063/1.1710367
[@ramo]: https://doi.org/10.1109/JRPROC.1939.228757
