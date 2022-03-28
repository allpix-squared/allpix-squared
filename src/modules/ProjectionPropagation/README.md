---
# SPDX-FileCopyrightText: 2017-2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "ProjectionPropagation"
description: "Propagation via projection to the sensor surface"
module_maintainer: "Simon Spannagel (<simon.spannagel@cern.ch>), Paul Schuetze (<paul.schuetze@desy.de>)"
module_status: "Functional"
module_input: "DepositedCharge"
module_output: "PropagatedCharge"
---

## Description
The module projects the deposited electrons (or holes) to the sensor surface and applies a randomized, simplified diffusion. It can be used to save computing time at the cost of precision.

The diffusion of the charge carriers is realized by placing sets of a configurable number of electrons in positions drawn as a random number from a two-dimensional Gaussian distribution around the projected position at the sensor surface. The diffusion width is based on an approximation of the drift time, using an analytical approximation for the integral of the mobility in a linear electric field. Here, the charge carrier mobility parametrization of Jacoboni \[[@jacoboni]\] is used. The integral is calculated as follows, with $` \mu_0 = V_m/E_c `$:

$` t = \int\frac 1v ds = \int \frac{1}{\mu(s)E(s)} ds = \int \frac{\left(1+\left(\frac{E(S)}{E_c}\right)^\beta\right)^{1/\beta}}{\mu_0E(s)} ds `$

Here, $` \beta `$ is set to 1, inducing systematic errors less than 10%, depending on the sensor temperature configured. With the linear approximation to the electric field as $`E(s) = ks+E_0`$ it is

$` t = \frac {1}{\mu_0}\int\left( \frac{1}{E(s)} + \frac{1}{E_c} \right) ds = \frac {1}{\mu_0}\int\left( \frac{1}{ks+E_0} + \frac{1}{E_c} \right) ds = \frac {1}{\mu_0}\left[ \frac{\ln(ks+E_0)}{k} + \frac{s}{E_c} \right]^b _a = \frac{1}{\mu_0} \left[ \frac{\ln(E(s))}{k} + \frac{s}{E_c} \right]^b _a`$.

Since the approximation of the drift time assumes a linear electric field, this module cannot be used with any other electric field configuration.

Depending on the parameter `diffuse_deposit`, deposited charge carriers in a sensor region without electric field are either not propagated, or a single, three-dimensional diffusion step prior to the propagation of these charge carriers, corresponding to the `integration_time` is enabled.
Charge carriers diffusing into the electric field will be placed at the border between the undepleted and the depleted regions with the corresponding offset in time and then be propagated to the sensor surface.

The charge carrier lifetime can be simulated using the doping concentration of the sensor. The recombination model is selected via the `recombination_model` parameter, the default value `none` is equivalent to not simulating finite lifetimes. This feature can only be enabled if a doping profile has been loaded for the respective detector using the DopingProfileReader module. This module only supports doping profiles of type **constant**.
The doping-dependent charge carrier lifetime is determined once and the survival probability is calculated by drawing a random number from an uniform distribution with $`0 \leq r \leq 1`$ and comparing it to the expression $`t/\tau`$, where $`t`$ is the total propagation time of the charge carrier to the sensor surface.
Charge carriers which would recombine before reaching the surface are removed from the simulation.

Lorentz drift in a magnetic field is not supported. Hence, in order to use this module with a magnetic field present, the parameter `ignore_magnetic_field` can be set.

## Parameters
* `temperature`: Temperature in the sensitive device, used to estimate the diffusion constant and therefore the width of the diffusion distribution.
* `recombination_model`: Charge carrier lifetime model to be used for the propagation. Defaults to `none`, a list of available models can be found in the documentation. This feature requires a doping concentration to be present for the detector.
* `charge_per_step`: Maximum number of electrons placed for which the randomized diffusion is calculated together, i.e. they are placed at the same position. Defaults to 10.
* `propagate_holes`: If set to `true`, holes are propagated instead of electrons. Defaults to `false`. Only one carrier type can be selected since all charges are propagated towards the implants.
* `ignore_magnetic_field`: Enables the usage of this module with a magnetic field present, resulting in an unphysical propagation w/o Lorentz drift. Defaults to false.
* `integration_time` : Time within which charge carriers are propagated. If the total drift time exceeds, the respective carriers are ignored and do not contribute to the signal. Defaults to the LHC bunch crossing time of 25ns.
* `diffuse_deposit`: Enables a diffusion prior to the propagation for charge carriers deposited in a region without electric field. Defaults to `false`.
* `output_plots`: Determines if plots should be generated.


## Usage
```
[ProjectionPropagation]
temperature = 293K
charge_per_step = 10
output_plots = 1
```

[@jacoboni]: https://doi.org/10.1016/0038-1101(77)90054-5
