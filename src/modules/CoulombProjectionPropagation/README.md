---
# SPDX-FileCopyrightText: 2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "CoulombProjectionPropagation"
description: "Projects deposited charges to the sensor surface including the Coulomb repulsion via a parameterized distribution"
module_status: "Immature"
module_maintainers: ["Xiangyu Xie (<xiangyu.xie@psi.ch>)"]
module_inputs: ["DepositedCharge"]
module_outputs: ["PropagatedCharge"]
---

## Description

This module projects the deposited electrons (or holes) to the sensor surface, similar to the ProjectionPropagation module, but includes the mutual Coulomb repulsion of the charge carriers in addition to drift and diffusion. The repulsion is important for dense charge clouds, e.g. from the absorption of X-rays with energies above a few keV, where it broadens the charge cloud and increases the charge sharing between pixels \[[@xie2025]\].

The lateral distribution of the collected charge carriers is described by a Generalized Gaussian Distribution (GGD) with scale parameter $`\alpha`$ and shape parameter $`\beta`$:

```math
f(x) = \frac{\beta}{2 \alpha \Gamma(1/\beta)} \exp\left(-\left(\frac{|x|}{\alpha}\right)^\beta\right)
```

A value of $`\beta = 2`$ corresponds to a Gaussian distribution, while $`\beta > 2`$ indicates the broader peak caused by the repulsion. Both parameters are parameterized as functions of the approximated drift time $`t`$:

```math
\begin{aligned}
\alpha(t) &= a_0 + a_1 \sqrt{t} + a_2 t + a_3 t^2 \\
\beta(t) &= 2 + b_0 (t - b_1)^{b_2} + b_3 \exp(b_4 t)
\end{aligned}
```

with $`\alpha`$ in um and $`t`$ in ns. The nine parameters $`a_0`$ to $`a_3`$ and $`b_0`$ to $`b_4`$ depend on the photon energy, sensor thickness, bias and depletion voltage and temperature. They are obtained by fitting the results of a dedicated charge transport simulation with repulsion at different absorption depths, e.g. with the tools available in \[[@cts]\], and are provided via the `parameters` configuration key.

The drift time is approximated as in the ProjectionPropagation module, using the mobility parameterization of Jacoboni \[[@jacoboni]\] with $`\beta = 1`$ in a linear electric field:

```math
t = \frac{1}{\mu_0} \left[ \frac{\ln(E(z))}{k} + \frac{z}{E_c} \right]^{z_\text{implant}}_{z_0}
```

where $`k`$ is the slope of the electric field and $`z_0`$ the depth of the deposit. The parameterization describes the whole charge cloud created by an absorbed particle as function of its absorption depth. For X-ray photons, all deposits of the photoelectron lie within a few micrometers of the absorption point, such that the drift time calculated for the individual deposits differs negligibly from the one of the absorption point.

The charge carriers of each deposit are split into groups of `charge_per_step` carriers, which are placed at positions drawn from the GGD in $`x`$ and $`y`$ around the deposit position on the implant side of the sensor. Charge carriers with a drift time exceeding the integration time or ending up outside the sensor are discarded.

Since the drift time approximation assumes a linear electric field, this module cannot be used with any other electric field configuration. Lorentz drift in a magnetic field is not supported. Hence, in order to use this module with a magnetic field present, the parameter `ignore_magnetic_field` can be set.

## Parameters

* `parameters`: Array of the nine parameters $`a_0, a_1, a_2, a_3, b_0, b_1, b_2, b_3, b_4`$ of $`\alpha(t)`$ and $`\beta(t)`$, with $`\alpha`$ in um and $`t`$ in ns. Required.
* `temperature`: Temperature of the sensitive device, used for the mobility in the drift time approximation. Needs to be the temperature used for the parameterization. Required.
* `charge_per_step`: Maximum number of charge carriers placed at the same position. Defaults to 10.
* `max_charge_groups`: Maximum number of charge groups to propagate from a single deposit point. Temporarily increases the value of `charge_per_step` to reduce the number of propagated groups if the deposit is larger than the value `max_charge_groups`*`charge_per_step`. The default value is 1000 charge groups. If it is set to 0, there is no upper limit on the number of charge groups propagated.
* `propagate_holes`: If set to `true`, holes are propagated instead of electrons. Defaults to `false`. Only one carrier type can be selected since all charges are propagated towards the implants.
* `ignore_magnetic_field`: Enables the usage of this module with a magnetic field present, resulting in an unphysical propagation w/o Lorentz drift. Defaults to `false`.
* `integration_time`: Time within which charge carriers are propagated. If the drift time exceeds it, the respective carriers are ignored. Defaults to the LHC bunch crossing time of 25 ns.

## Plotting parameters

* `output_plots`: Determines if simple output plots should be generated for a monitoring of the simulation flow. Disabled by default.

## Usage

Parameterization for 15 keV photons in a 320 um thick silicon sensor at 150 V bias voltage, 29.6 V depletion voltage and 307.8 K:

```ini
[CoulombProjectionPropagation]
temperature = 307.8K
propagate_holes = true
parameters = 0.4112, 2.979, -0.232, 0.003955, 1.348, -3.399, -0.5348, -1.091, -19.82
```

[@xie2025]: https://doi.org/10.1016/j.nima.2025.170894
[@cts]: https://github.com/slsdetectorgroup/ChargeTransportSimulation
[@jacoboni]: https://doi.org/10.1016/0038-1101(77)90054-5
