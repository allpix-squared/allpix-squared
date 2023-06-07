---
# SPDX-FileCopyrightText: 2017-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "TransientPropagation"
description: "Propagation of deposited charges via Shockley-Ramo induction"
module_maintainer: "Simon Spannagel (<simon.spannagel@cern.ch>)"
module_status: "Functional"
module_input: "DepositedCharge"
module_ouput: "PropagatedCharge"
---

## Description
Simulates the transport of electrons and holes through the sensitive sensor volume of the detector. It allows to propagate sets of charge carriers together in order to speed up the simulation while maintaining the required accuracy. The propagation process for these sets is fully independent and no interaction is simulated. The maximum size of the set of propagated charges and thus the accuracy of the propagation can be controlled via the `charge_per_step` parameter. The maximum number of charge groups to be propagated for a single deposit position can be controlled via the `max_charge_groups` parameter.

The propagation consists of a combination of drift and diffusion simulation. The drift is calculated using the charge carrier velocity derived from the charge carrier mobility and the magnetic field via a calculation of the Lorentz drift. The mobility model can be chosen using the `mobility_model` parameter, and a list of available models can be found in the user manual. If the `masetti` or `masetti_canali` is used, the `dopant_n` parameter can be used to set the n-dopant to either phosphorous (default) or arsenic.
This module implements charge multiplication by impact ionization. The multiplication model can be chosen using the `multiplication_model` parameter, the list of available models can be found in the user manual. By default, the model defaults to `none` and impact ionization is switched off, generating unity gain.
To simulate impact ionization, the number of newly generated electron-hole pairs is calculated for every propagation step and every charge carrier in the group, based on drawing a random number from a geometric distribution. This represents a stepwise approach to the avalanche generation process. The charge of a charge group is increased by the number of impact ionization processes per step and opposite-type charge carriers are generated at the end of the step.

A fourth-order Runge-Kutta-Fehlberg method \[[@fehlberg]\] is used to integrate the particle motion through the electric and magnetic fields. After every Runge-Kutta step, the diffusion is accounted for by applying an offset drawn from a Gaussian distribution calculated from the Einstein relation

$`\sigma = \sqrt{\frac{2k_b T}{e}\mu t}`$

using the carrier mobility $`\mu`$, the temperature $`T`$ and the time step $`t`$. The propagation stops when the set of charges reaches any surface of the sensor.

The charge transport is parameterized in time and the time step each simulation step takes can be configured.
For each step, the induced charge on the neighboring pixel implants is calculated via the Shockley-Ramo theorem \[[@shockley], [@ramo]\] by taking the difference in weighting potential between the current position $`x_1`$ and the previous position $`x_0`$ of the charge carrier

$` Q_n^{ind}  = \int_{t_0}^{t_1} I_n^{ind} = q \left( \phi (x_1) - \phi(x_0) \right)`$

and multiplying it with the charge. The resulting pulses are stored for every set of charge carriers individually and need to be combined for each pixel using a transfer module.

The charge carrier lifetime can be simulated using the doping concentration of the sensor. The recombination model is selected via the `recombination_model` parameter, the default value `none` is equivalent to not simulating finite lifetimes. This feature can only be enabled if a doping profile has been loaded for the respective detector using the DopingProfileReader module.
In each step, the doping-dependent charge carrier lifetime is determined, from which a survival probability is calculated.
The survival probability is calculated at each step of the propagation by drawing a random number from an uniform distribution with $`0 \leq r \leq 1`$ and comparing it to the expression $`dt/\tau`$, where $`dt`$ is the time step of the last charge carrier movement.

Trapping of charge carriers can be enabled by setting a trapping model via the parameter `trapping_model`.
The default value is `none`, corresponding to no charge carrier trapping being simulated.
All models require the 1MeV-neutron equivalent fluence to be set via the parameter `fluence`.
Some models include temperature-dependent scaling of trapping probabilities, and the corresponding temperature is taken from the `temperature` parameter.
The trapping probability is calculated at each step of the propagation by drawing a random number from an uniform distribution with $`0 \leq r \leq 1`$ and comparing it to the expression $`1 - e^{-dt/\tau_{eff}}`$, where $`dt`$ is the time step of the last charge carrier movement and $`\tau_{eff}`$ the effective trapping time constant.
A list of available models can be found in the user manual.

Detrapping of charge carriers can be enabled by setting a detrapping model via the parameter `detrapping_model`.
The default value is `none`, corresponding to no charge carrier detrapping being simulated.
A list of available models can be found in the user manual.

The module can produces a variety of plots such as total integrated charge plots as well as histograms on the step length and observed potential differences. Furthermore, the module can generate a 3D line plot of the path of all separately propagated charge carrier sets from their point of deposition to the end of their drift, with nearby paths having different colors. In this coloring scheme, electrons are marked in blue colors, while holes are presented in different shades of orange.
In addition, a 3D GIF animation for the drift of all individual sets of charges (with the size of the point proportional to the number of charges in the set) can be produced. Finally, the module produces 2D contour animations in all the planes normal to the X, Y and Z axis, showing the concentration flow in the sensor.
It should be noted that generating the animations is time-consuming and should be switched off even when investigating drift behavior.

## Parameters
* `temperature`: Temperature of the sensitive device, used to estimate the diffusion constant and therefore the strength of the diffusion. Defaults to room temperature (293.15K).
* `mobility_model`: Charge carrier mobility model to be used for the propagation. Defaults to `jacoboni`, a list of available models can be found in the documentation.
* `recombination_model`: Charge carrier lifetime model to be used for the propagation. Defaults to `none`, a list of available models can be found in the documentation. This feature requires a doping concentration to be present for the detector.
* `trapping_model`: Model for simulating charge carrier trapping from radiation-induced damage. Defaults to `none`, a list of available models can be found in the documentation. All models require explicitly setting a fluence parameter.
* `detrapping_model`: Model for simulating charge carrier detrapping from radiation-induced damage. Defaults to `none`, a list of available models can be found in the documentation.
* `fluence`: 1MeV-neutron equivalent fluence the sensor has been exposed to.
* `charge_per_step`: Maximum number of charge carriers to propagate together. Divides the total number of deposited charge carriers at a specific point into sets of this number of charge carriers and a set with the remaining charge carriers. A value of 10 charges per step is used by default if this value is not specified.
* `max_charge_groups`: Maximum number of charge groups to propagate from a single deposit point. Temporarily increases the value of `charge_per_step` to reduce the number of propagated groups if the deposit is larger than the value `max_charge_groups`*`charge_per_step`, thus reducing the negative performance impact of unexpectedly large deposits. The default value is 1000 charge groups. If it is set to 0, there is no upper limit on the number of charge groups propagated.
* `timestep`: Time step for the Runge-Kutta integration, representing the granularity with which the induced charge is calculated. Default value is 0.01ns.
* `integration_time`: Time within which charge carriers are propagated. After exceeding this time, no further propagation is performed for the respective carriers. Defaults to the LHC bunch crossing time of 25ns.
* `distance`: Maximum distance of pixels to be considered for current induction, calculated from the pixel the charge carrier under investigation is below. A distance of `1` for example means that the induced current for the closest pixel plus all neighbors is calculated. It should be noted that the time required for simulating a single event depends almost linearly on the number of pixels the induced charge is calculated for. Usually, for Cartesian sensors a 3x3 grid (9 pixels, distance 1) should suffice since the weighting potential at a distance of more than one pixel pitch often is small enough to be neglected while the simulation time is almost tripled for `distance = 2` (5x5 grid, 25 pixels). To just calculate the induced current in the one pixel the charge carrier is below, `distance = 0` can be used. Defaults to `1`.
* `ignore_magnetic_field`: The magnetic field, if present, is ignored for this module. Defaults to false.
* `multiplication_model`: Model used to calculate impact ionization parameters and charge multiplication. Defaults to `none` which corresponds to unity gain, a list of available models can be found in the documentation.
* `multiplication_threshold`: Threshold field above which charge multiplication is calculated. Defaults to `100kV/cm`.
* `max_multiplication_level`: Maximum level depth of the generated impact ionization charge multiplication shower after which the generation of further multiplication charge carrier levels is prohibited. This number represents the maximum number of daughter charge carrier groups that can be produced by one initial charge carrier group. This does not concern the size of the charge group itself but solely the level of generation. If a group generates a secondary group through impact ionization, the depth is `1`. If this secondary group again creates charge carriers when propagating, the level is `2` and so on. The default value is `5`.


## Plotting parameters
* `output_plots` : Determines if simple output plots should be generated for a monitoring of the simulation flow. Disabled by default.
* `output_linegraphs` : Determines if line graphs should be generated for every event. This causes a significant slow down of the simulation, it is not recommended to enable this option for runs with more than a couple of events. Disabled by default.
* `output_linegraphs_collected` : Determine whether to also generate line graphs *only* for charge carriers that have reached the implant side within the allotted integration time. Defaults to `false`. This requires `output_linegraphs` to be active.
* `output_linegraphs_recombined` : Boolean flag to select whether line graphs should also be generated only from charge carriers that have recombined with the lattice during the integration time. Defaults to `false`. This requires `output_linegraphs` to be active.
* `output_linegraphs_trapped` : Boolean flag to select whether line graphs should also be generated only from charge carriers that have been trapped during their motion through the sensor. Defaults to `false`. This requires `output_linegraphs` to be active.
* `output_plots_step` : Timestep to use between two points plotted. Indirectly determines the amount of points plotted. Defaults to *timestep_max* if not explicitly specified.
* `output_plots_theta` : Viewpoint angle of the 3D animation and the 3D line graph around the world X-axis. Defaults to zero.
* `output_plots_phi` : Viewpoint angle of the 3D animation and the 3D line graph around the world Z-axis. Defaults to zero.
* `output_plots_use_pixel_units` : Determines if the plots should use pixels as unit instead of metric length scales. Defaults to false (thus using the metric system).
* `output_plots_use_equal_scaling` : Determines if the plots should be produced with equal distance scales on every axis (also if this implies that some points will fall out of the graph). Defaults to true.
* `output_plots_align_pixels` : Determines if the plot should be aligned on pixels, defaults to false. If enabled the start and the end of the axis will be at the split point between pixels.
* `output_animations` : In addition to the other output plots, also write a GIF animation of the charges drifting towards the electrodes. This is extremely slow and writing the animation takes a considerable amount of time, therefore defaults to false. This option also requires `output_linegraphs` to be enabled.
* `output_animations_time_scaling` : Scaling for the animation used to convert the actual simulation time to the time step in the animation. Defaults to 1.0e9, meaning that every nanosecond of the simulation is equal to an animation step of a single second.
* `output_animations_marker_size` : Scaling for the markers on the animation, defaults to one. The markers are already internally scaled to the charge of their step, normalized to the maximum charge.
* `output_animations_contour_max_scaling` : Scaling to use for the contour color axis from the theoretical maximum charge at every single plot step. Default is 10, meaning that the maximum of the color scale axis is equal to the total amount of charges divided by ten (values above this are displayed in the same maximum color). Parameter can be used to improve the color scale of the contour plots.
* `output_animations_color_markers`: Determines if colors should be for the markers in the animations, defaults to false.

## Usage
```ini
[TransientPropagation]
temperature = 293K
charge_per_step = 10
output_plots = true
timestep = 0.02ns
```

[@fehlberg]: https://ntrs.nasa.gov/search.jsp?R=19690021375
[@shockley]: https://doi.org/10.1063/1.1710367
[@ramo]: https://doi.org/10.1109/JRPROC.1939.228757
