---
# SPDX-FileCopyrightText: 2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "SphericalRepulsionPropagation"
description: "Propagates charge clouds including drift, diffusion and Coulomb repulsion using a spherical model"
module_status: "Immature"
module_maintainers: ["Xiangyu Xie (<xiangyu.xie@psi.ch>)"]
module_inputs: ["DepositedCharge"]
module_outputs: ["PropagatedCharge"]
---

## Description

This module propagates charge carriers to the sensor surface taking into account drift, diffusion and the mutual Coulomb repulsion of the carriers. It implements the simplified spherical model described in \[[@xie2025]\] with a time-stepping Monte Carlo simulation, which is performed for every event without any precomputed parameters. The repulsion is important for dense charge clouds, e.g. from the absorption of X-rays with energies above a few keV, where it broadens the charge cloud significantly and hence increases the charge sharing between pixels.

### Charge cloud

All deposits of the same carrier type in an event form one charge cloud, centered at their charge-weighted centroid. This corresponds to the absorption of a single X-ray photon per event. The carriers are initially distributed following a three-dimensional Gaussian distribution around the cloud center, with a width derived from the Bethe range $`R`$ of the photoelectron in silicon \[[@everhart], [@fitting]\]:

```math
\sigma = R / \sqrt{15} = 0.0044\,\mu\text{m} \cdot (E/\text{keV})^{1.75}
```

where the cloud energy $`E`$ is the total cloud charge multiplied by `charge_creation_energy`. This parameterization has been validated for photon energies between 5 keV and 25 keV in silicon; a warning is printed for clouds outside this range. The spatial distribution of the individual deposits is not used. Electron and hole clouds are propagated independently, their mutual attraction is neglected.

### Propagation

The cloud center $`\vec{c}`$ drifts in the electric field at its position, while the positions $`\vec{r}_i`$ of the carriers relative to the center change by diffusion and repulsion. Assuming spherical symmetry of the cloud, the repulsion field at carrier $`i`$ follows from Gauss' law using the charge $`Q_i`$ enclosed by the sphere through the carrier:

```math
\vec{E}_{\text{rep},i} = \frac{Q_i}{4 \pi \epsilon_0 \epsilon_r r_i^2} \hat{r}_i
```

The carriers are sorted by their distance to the cloud center in every step, which reduces the computational complexity from $`O(N^2)`$ for pairwise calculations to $`O(N \log N)`$. For every time step $`\delta t`$ the positions are updated as

```math
\begin{aligned}
\vec{r}_i &\leftarrow \vec{r}_i + \mu_i \vec{E}_{\text{rep},i} \delta t + \sqrt{2 D_i \delta t} \cdot \vec{g} \\
\vec{c} &\leftarrow \vec{c} \pm \langle \mu \rangle \vec{E}_{\text{drift}}(\vec{c}) \delta t
\end{aligned}
```

where $`\vec{g}`$ is a vector of standard normal random numbers, $`D_i = \mu_i k T / e`$ is the diffusion coefficient from the Einstein relation and $`\langle \mu \rangle`$ is the charge-weighted mean mobility of the cloud. The mobility $`\mu_i`$ of each carrier is calculated from the magnitude of the total field $`|\vec{E}_{\text{drift}} + \vec{E}_{\text{rep},i}|`$ using the selected mobility model.

The propagation ends once the cloud center reaches the sensor surface it drifts towards. All carriers are then placed on this surface at their lateral positions and are marked as halted. If the integration time is reached before, the carriers are stored at their current positions in state motion. Charge carriers which end up outside the sensor are discarded.

### Grouping of charge carriers

To reduce the computational cost, the cloud is rebinned into groups of `charge_per_step` carriers, which are sampled from the initial Gaussian distribution and propagated together. Each group is linked to the deposit its first carrier originates from to retain the Monte Carlo history. A group of $`q`$ carriers sees on average $`(q-1)/2`$ of its own carriers inside its shell, such that the enclosed charge is $`Q_i = \sum_{r_j < r_i} q_j + (q_i - 1)/2`$. This reduces to the exact expression for single carriers and keeps the repulsion strength independent of the group size. For 25 keV photons in 320 um thick silicon, the lateral width of the collected cloud changes by less than 1% between groups of 1 and 100 carriers, while the computing time is reduced by a factor of four.

Lorentz drift in a magnetic field is not supported. Hence, in order to use this module with a magnetic field present, the parameter `ignore_magnetic_field` can be set.

## Parameters

* `temperature`: Temperature of the sensitive device, used to calculate the mobility and the diffusion coefficient. Defaults to 293.15 K.
* `mobility_model`: Charge carrier mobility model to be used for the propagation. Defaults to `jacoboni`, a list of available models can be found in the documentation.
* `timestep`: Time step of the propagation. Defaults to 10 ps.
* `integration_time`: Time within which charge carriers are propagated. Defaults to the LHC bunch crossing time of 25 ns.
* `charge_per_step`: Number of charge carriers which are propagated together as one group. Defaults to 5.
* `max_charge_groups`: Maximum number of charge groups to propagate per cloud. Temporarily increases the value of `charge_per_step` if a cloud exceeds `max_charge_groups` * `charge_per_step` carriers. Defaults to 0, i.e. no upper limit on the number of groups.
* `propagate_electrons`: Select whether electrons should be propagated. Defaults to `true`.
* `propagate_holes`: Select whether holes should be propagated. Defaults to `false`.
* `charge_creation_energy`: Energy needed to create one electron-hole pair, used to calculate the cloud energy for the initial Gaussian distribution. Defaults to the value of the sensor material, e.g. 3.64 eV for silicon.
* `relative_permittivity`: Relative permittivity of the sensor material. Defaults to 11.7 for silicon, needs to be specified for other materials.
* `ignore_magnetic_field`: Enables the usage of this module with a magnetic field present, resulting in an unphysical propagation w/o Lorentz drift. Defaults to `false`.

## Plotting parameters

* `output_plots`: Determines if output plots should be generated. These comprise the drift time of the clouds, the charge per cloud, the charge carrier group size and the lateral RMS per axis of the collected clouds as function of their initial position. Disabled by default.

## Usage

```ini
[SphericalRepulsionPropagation]
temperature = 307.85K
propagate_electrons = false
propagate_holes = true
charge_per_step = 5
```

[@xie2025]: https://doi.org/10.1016/j.nima.2025.170894
[@everhart]: https://doi.org/10.1063/1.1660019
[@fitting]: https://doi.org/10.1002/pssa.2210430119
