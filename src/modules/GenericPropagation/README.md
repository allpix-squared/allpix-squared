# GenericPropagation
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  
**Input**: DepositedCharge  
**Output**: PropagatedCharge

### Description
Simulates the propagation of electrons and/or holes through the sensitive sensor volume of the detector. It allows to propagate sets of charge carriers together in order to speed up the simulation while maintaining the required accuracy. The propagation process for these sets is fully independent and no interaction is simulated. The maximum size of the set of propagated charges and thus the accuracy of the propagation can be controlled.

The propagation consists of a combination of drift and diffusion simulation. The drift is calculated using the charge carrier velocity derived from the charge carrier mobility and the magnetic field via a calculation of the Lorentz drift. The correct mobility for either electrons or holes is automatically chosen, based on the type of the charge carrier under consideration. Thus, also input with both electrons and holes is treated properly. The mobility model can be chosen using the `mobility_model` parameter, and a list of available models can be found in the user manual.

The two parameters `propagate_electrons` and `propagate_holes` allow to control which type of charge carrier is propagated to their respective electrodes. Either one of the carrier types can be selected, or both can be propagated. It should be noted that this will slow down the simulation considerably since twice as many carriers have to be handled and it should only be used where sensible.
The direction of the propagation depends on the electric and magnetic fields field configured, and it should be ensured that the carrier types selected are actually transported to the implant side. For linear electric fields, a warning is issued if a possible misconfiguration is detected.

A fourth-order Runge-Kutta-Fehlberg method [@fehlberg] with fifth-order error estimation is used to integrate the particle propagation in the electric and magnetic fields. After every Runge-Kutta step, the diffusion is accounted for by applying an offset drawn from a Gaussian distribution calculated from the Einstein relation

$`\sigma = \sqrt{\frac{2k_b T}{e}\mu t}`$

using the carrier mobility $`\mu`$, the temperature $`T`$ and the time step $`t`$. The propagation stops when the set of charges reaches any surface of the sensor.

The charge carrier life time can be simulated using the doping concentration of the sensor. This feature is only enabled if a doping profile is loaded for the respective detector using the DopingProfileReader module.
The life time $`\tau_{srh}`$ is then calculated using the Shockley-Read-Hall relation [@fossum-lee]

$`\tau_{srh} = \frac{\tau_0}{1 + \frac{N_d}{N_{d0}}}`$

where $`\tau_0`$ and $`N_{d0}`$ are reference life time and doping concentration taken from literature [@fossum].
In addition, the charge carrier life time $`\tau_{a}`$ according to the Auger recombination model is calculated [@haug]

$`\tau_{a} = \frac{1}{C_{a}N_{d}}`$

where $`C_{a}`$ is the Auger coefficient.
The effective life time is then given by

$`\tau^{-1} = \tau_{srh}^{-1} + \tau_{a}^{-1}`$.

In each step, the doping-dependent charge carrier life time is determined, from which a survival probability is calculated.
The survival probability is calculated at each step of the propagation by drawing a random number from an uniform distribution with $`0 \leq r \leq 1`$ and comparing it to the expression $`t/\tau`$, where $`t`$ is the time since the creation of the charge carrier.

The propagation module also produces a variety of output plots. These include a 3D line plot of the path of all separately propagated charge carrier sets from their point of deposition to the end of their drift, with nearby paths having different colors. In this coloring scheme, electrons are marked in blue colors, while holes are presented in different shades of orange.
In addition, a 3D GIF animation for the drift of all individual sets of charges (with the size of the point proportional to the number of charges in the set) can be produced. Finally, the module produces 2D contour animations in all the planes normal to the X, Y and Z axis, showing the concentration flow in the sensor.
It should be noted that generating the animations is very time-consuming and should be switched off even when investigating drift behavior.

### Dependencies

This module requires an installation of Eigen3.

### Parameters
* `temperature` : Temperature of the sensitive device, used to estimate the diffusion constant and therefore the strength of the diffusion. Defaults to room temperature (293.15K).
* `mobility_model`: Charge carrier mobility model to be used for the propagation. Defaults to `jacoboni`, a list of available models can be found in the documentation.
* `charge_per_step` : Maximum number of charge carriers to propagate together. Divides the total number of deposited charge carriers at a specific point into sets of this number of charge carriers and a set with the remaining charge carriers. A value of 10 charges per step is used by default if this value is not specified.
* `spatial_precision` : Spatial precision to aim for. The timestep of the Runge-Kutta propagation is adjusted to reach this spatial precision after calculating the uncertainty from the fifth-order error method. Defaults to 0.25nm.
* `timestep_start` : Timestep to initialize the Runge-Kutta integration with. Appropriate initialization of this parameter reduces the time to optimize the timestep to the *spatial_precision* parameter. Default value is 0.01ns.
* `timestep_min` : Minimum step in time to use for the Runge-Kutta integration regardless of the spatial precision. Defaults to 1ps.
* `timestep_max` : Maximum step in time to use for the Runge-Kutta integration regardless of the spatial precision. Defaults to 0.5ns.
* `integration_time` : Time within which charge carriers are propagated. After exceeding this time, no further propagation is performed for the respective carriers. Defaults to the LHC bunch crossing time of 25ns.
* `propagate_electrons` : Select whether electron-type charge carriers should be propagated to the electrodes. Defaults to true.
* `propagate_holes` :  Select whether hole-type charge carriers should be propagated to the electrodes. Defaults to false.
* `ignore_magnetic_field`: The magnetic field, if present, is ignored for this module. Defaults to false.
* `auger_coefficient` : Auger coefficient for the Auger recombination model. Default is $`2 \times 10^{-30}\,\mathrm{cm}^{6}\,\mathrm{s}^{-1}"`$.

### Plotting parameters
* `output_plots` : Determines if simple output plots should be generated for a monitoring of the simulation flow. Disabled by default.
* `output_linegraphs` : Determines if linegraphs should be generated for every event. This causes a significant slow down of the simulation, it is not recommended to enable this option for runs with more than a couple of events. Disabled by default.
* `output_plots_step` : Timestep to use between two points plotted. Indirectly determines the amount of points plotted. Defaults to *timestep_max* if not explicitly specified.
* `output_plots_theta` : Viewpoint angle of the 3D animation and the 3D line graph around the world X-axis. Defaults to zero.
* `output_plots_phi` : Viewpoint angle of the 3D animation and the 3D line graph around the world Z-axis. Defaults to zero.
* `output_plots_use_pixel_units` : Determines if the plots should use pixels as unit instead of metric length scales. Defaults to false (thus using the metric system).
* `output_plots_use_equal_scaling` : Determines if the plots should be produced with equal distance scales on every axis (also if this implies that some points will fall out of the graph). Defaults to true.
* `output_plots_align_pixels` : Determines if the plot should be aligned on pixels, defaults to false. If enabled the start and the end of the axis will be at the split point between pixels.
* `output_plots_lines_at_implants` : Determine whether to plot all charge carrier drift lines (`false`) or to just plot lines from charge carriers which reached the implant side within the allotted integration time (`true`). Defaults to `false`, i.e. all charge carrier drift lines are drawn.
* `output_animations` : In addition to the other output plots, also write a GIF animation of the charges drifting towards the electrodes. This is very slow and writing the animation takes a considerable amount of time, therefore defaults to false. This option also requires `output_linegraphs` to be enabled.
* `output_animations_time_scaling` : Scaling for the animation used to convert the actual simulation time to the time step in the animation. Defaults to 1.0e9, meaning that every nanosecond of the simulation is equal to an animation step of a single second.
* `output_animations_marker_size` : Scaling for the markers on the animation, defaults to one. The markers are already internally scaled to the charge of their step, normalized to the maximum charge.
* `output_animations_contour_max_scaling` : Scaling to use for the contour color axis from the theoretical maximum charge at every single plot step. Default is 10, meaning that the maximum of the color scale axis is equal to the total amount of charges divided by ten (values above this are displayed in the same maximum color). Parameter can be used to improve the color scale of the contour plots.
* `output_animations_color_markers`: Determines if colors should be for the markers in the animations, defaults to false.

### Usage
A example of generic propagation for all sensors of type _Timepix_ at room temperature using packets of 25 charges is the following:

```toml
[GenericPropagation]
type = "timepix"
temperature = 293K
charge_per_step = 25
```

[@fehlberg]: https://ntrs.nasa.gov/search.jsp?R=19690021375
[@fossum-lee]: https://doi.org/10.1016/0038-1101(82)90203-9
[@fossum]: https://doi.org/10.1016/0038-1101(76)90022-8
[@haug]: https://doi.org/10.1016/0038-1098(78)90646-4
