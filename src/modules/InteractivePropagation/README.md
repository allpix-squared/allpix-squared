---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "InteractivePropagation"
description: "A propagation module that includes the Coulomb interaction forces between charges."
module_status: "Immature"
module_maintainers: ["gkunkler"]
module_inputs: ["DepositedCharge"]
module_outputs: ["PropagatedCharge"]
---

## Description
A propagation module that includes the Coulomb interaction forces between charges. It is based heavily on the TransientPropagation module, though major refactors were required to allow for the addition of the Coulomb component of the electric field. For simple detectors, it can also include the effect of mirror charges from electrodes held at constant potential on the top and bottom of the detector. While it takes significantly longer to run a simulation, the effects align better with experiments with large amounts of charge.

## Parameters

* `temperature`: Temperature of the sensitive device, used to estimate the diffusion constant and therefore the strength of the diffusion. Defaults to room temperature (293.15K).
* `mobility_model`: Charge carrier mobility model to be used for the propagation. Defaults to `jacoboni`, a list of available models can be found in the documentation.
* `recombination_model`: Charge carrier lifetime model to be used for the propagation. Defaults to `none`, a list of available models can be found in the documentation. This feature requires a doping concentration to be present for the detector.
* `trapping_model`: Model for simulating charge carrier trapping from radiation-induced damage. Defaults to `none`, a list of available models can be found in the documentation. All models require explicitly setting a fluence parameter.
* `detrapping_model`: Model for simulating charge carrier detrapping from radiation-induced damage. Defaults to `none`, a list of available models can be found in the documentation.
* `fluence`: 1MeV-neutron equivalent fluence the sensor has been exposed to.
* `charge_per_step`: Maximum number of charge carriers to propagate together. Divides the total number of deposited charge carriers at a specific point into sets of this number of charge carriers and a set with the remaining charge carriers. A value of 1 charges per step is used by default if this value is not specified.
* `max_charge_groups`: Maximum number of charge groups to propagate from all deposits. Increases the value of `charge_per_step` to reduce the number of propagated groups if the total amount of charge in all deposits is larger than the value `max_charge_groups * charge_per_step`, thus improving the consistency of limiting the total amount of charge. The default value is 1000 charge groups. Setting `max_charge_groups` to `0` will be interpreted as no maximum. The limit will often be exceeded slightly due to deposits with very few charges that can't be combined with others to reach the desired `charge_per_step`. The most number of charge groups that can occur is `max_charge_groups` + number of deposits. (Note that this choice of behavior is different from TransientPropagation since the simulation time is quadratically related to the total number of charges.)
* `timestep`: Time step for the Runge-Kutta integration, representing the granularity with which the induced charge is calculated. Default value is 0.01ns.
* `integration_time`: Time within which charge carriers are propagated. After exceeding this time, no further propagation is performed for the respective carriers. Defaults to the LHC bunch crossing time of 25ns.
* `distance`: Maximum distance of pixels to be considered for current induction, calculated from the pixel the charge carrier under investigation is below. A distance of `1` for example means that the induced current for the closest pixel plus all neighbors is calculated. It should be noted that the time required for simulating a single event depends almost linearly on the number of pixels the induced charge is calculated for. Usually, for Cartesian sensors a 3x3 grid (9 pixels, distance 1) should suffice since the weighting potential at a distance of more than one pixel pitch often is small enough to be neglected while the simulation time is almost tripled for `distance = 2` (5x5 grid, 25 pixels). To just calculate the induced current in the one pixel the charge carrier is below, `distance = 0` can be used. Defaults to `1`.
* `ignore_magnetic_field`: The magnetic field, if present, is ignored for this module. Defaults to false.
* `relative_permittivity`: The relative permittivity $\varepsilon_r$ of the sensor material. Determines the affect of the sensor material on the Coulomb field. Defaults to `1.0`.
* `coulomb_field_limit`: The maximum value for the electric field due to Coulomb repulsion between two charges. When the magnitude of the Coulomb field is greater than `coulomb_field_limit`, it is reduced to `coulomb_field_limit`. Defaults to 4e5V/cm.
* `coulomb_distance_limit`: The maximum distance a charge must be within to be included in the coulomb field calculation. Reduces the calculation of the field strength between charges further than this distance apart, but if set too large the extra if statement can make it take longer than without this parameter's implementation. Defaults to 4e-5cm.
* `enable_diffusion`: Whether diffusion occurs during propogation. Doesn't give physical results when disabled, so only use for verification of the propagation physics. Defaults to true.
* `enable_coulomb_repulsion`: Whether the electric field includes the Coulomb forces between charge groups. Simulation takes significantly longer when enabled. If set to false, performs approximately the same simulation that TransientPropagation does, so use that instead. Defaults to true.
* `propagate_electrons` : Select whether electron-type charge carriers should be propagated to the electrodes. Defaults to true.
* `propagate_holes` : Select whether hole-type charge carriers should be propagated to the electrodes. Defaults to true.
* `include_mirror_charges`: Select whether mirror charges should be included in the calculation of the coulomb field. Assumes that the detector is approximately a capacitor with constant potential on the top and bottom electrodes (in z). Additionally, the calculation of the electrode positions assumes that the detector origin is centered within it. Defaults to false.
* `multiplication_model`: Ignored in the present state.
* `multiplication_threshold`: Ignored in the present state
* `max_multiplication_level`: Ignored in the present state
* `surface_reflectivity`: Reflectivity of the sensor surface for charge carriers. Used to calculate a probability that charge carriers are not absorbed at the interface but reflected back into the sensor volume. Defaults to `0.0`, i.e. no reflectivity, and a value of `1.0` corresponds to total reflection.
relative_permativity_

## Plotting parameters

* `output_plots` : Determines if simple output plots should be generated for a monitoring of the simulation flow. Disabled by default.
* `output_linegraphs` : Determines if line graphs should be generated for every event. This causes a significant slow down of the simulation, it is not recommended to enable this option for runs with more than a couple of events. Disabled by default.
* `output_rms` : Determines if the rms of the charges should be calculated and plotted over time. Frequency of points is determined by `output_plots_step`.
* `output_linegraphs_collected` : Determine whether to also generate line graphs *only* for charge carriers that have reached the implant side within the allotted integration time. Defaults to `false`. This requires `output_linegraphs` to be active.
* `output_linegraphs_recombined` : Boolean flag to select whether line graphs should also be generated only from charge carriers that have recombined with the lattice during the integration time. Defaults to `false`. This requires `output_linegraphs` to be active.
* `output_linegraphs_trapped` : Boolean flag to select whether line graphs should also be generated only from charge carriers that have been trapped during their motion through the sensor. Defaults to `false`. This requires `output_linegraphs` to be active.
* `output_plots_step` : Timestep to use between two points plotted. Indirectly determines the amount of points plotted. Defaults to *timestep_max* if not explicitly specified. Also used to determine the time between integration update message as well as rms calculations.
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
[InteractivePropagation]
max_charge_groups = 1500 
charge_per_step = 30

integration_time = 50ns
timestep = 0.1ns
distance = 0
output_plots = 1
output_rms = 1
output_plots_step = 0.2ns

relative_permativity = 11.7 # Silicon

enable_diffusion = 1
enable_coulomb_repulsion = 1

propagate_electrons = 1
propagate_holes = 1
include_mirror_charges = 1

mobility_model = "jacoboni"
