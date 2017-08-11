## GenericPropagation
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)
**Status**: Functional
**Input**: DepositedCharge
**Output**: PropagatedCharge

#### Description
Simulates generic propagation of electrons (ignoring the corresponding holes) through the sensitive devices of every detector. Splits up the set of deposited charges in multiple smaller sets of charges (containing multiple charges) that are propagated together. The propagation process is fully independent, the individual sets of propagated charges do not influence each other. The maximum size of the set of propagated charges and the accuracy of the propagation can be controlled.

The propagation consists of a combination of drift and diffusion simulation. The drift is calculated using the charge carrier velocity derived from the electron mobility parameterization by C. Jacobini et al. in [A review of some charge transport properties of silicon](https://doi.org/10.1016/0038-1101(77)90054-5). The correct mobility for either electrons or holes is automatically chosen, based on the type of the charge carrier under consideration. Thus, also input with both electrons and holes is treated properly.

The two parameters `propagate_electrons` and `propagate_holes` allow to control, which type of charge carrier is propagated to their respective electrodes. Either one of the carrier types can be selected, or both can be propagated. It should be noted that this will slow down the simulation considerably since twice as many carriers have to be handled and it should only be used where sensible.
The direction of the propagation depends on the electric field configured, and it should be ensured that the carrier types selected are actually transported to the implant side.

An fourth-order Runge-Kutta-Fehlberg method with fifth-order error estimation is used to integrate the electric field. After every Runge-Kutta step a random walk is simulated by applying Gaussian diffusion calculated from the electron mobility, the temperature and the time step. The propagation stops when the set of charges reaches the border of the sensor.


The propagation module also produces a variety of output plots for debugging and publication purposes. The plots include a 3D line plot of the path of all separate propagated charges from their deposits, with nearby paths having different colors. In this coloring scheme, electrons are marked in blue colors, while holes are presented in different shades of orange.
In addition, a 3D GIF animation for the drift of all individual sets of charges (with the size of the point proportional to the number of charges in the set) can be produced. Finally, the module produces 2D contour animations in all the planes normal to the X, Y and Z axis, showing the concentration flow in the sensor.
It should be noted that generating the animations is very time-consuming and should be switched off even when investigating drift behavior.

#### Parameters
* `temperature` : Temperature in the sensitive device, used to estimate the diffusion constant and therefore the strength of the diffusion.
* `charge_per_step` : Maximum number of charges to propagate together. Divides the total deposited charge at a specific point in sets of this number of charges and a set with the remaining amount of charges. A value of 10 charges per step is used if this value is not specified.
* `spatial_precision` : Spatial precision to aim for. The timestep of the Runge-Kutta propagation is adjusted to reach this spatial precision after calculating the error from the fifth-order error method. Defaults to 0.1nm.
* `timestep_start` : Timestep to initialize the Runge-Kutta integration with. Better initialization of this parameter reduces the time to optimize the timestep to the *spatial_precision* parameter. Default value is 0.01ns.
* `timestep_min` : Minimum step in time to use for the Runge-Kutta integration regardless of the spatial precision. Defaults to 0.5ps.
* `timestep_max` : Maximum step in time to use for the Runge-Kutta integration regardless of the spatial precision. Defaults to 0.1ns.
* `integration_time` : Time within which charge carriers are propagated. After exceeding this time, no further propagation is performed for the respective carriers. Defaults to the LHC bunch crossing time of 25ns.
* `propagate_electrons` : Select whether electron-type charge carriers should be propagated to the electrodes. Defaults to true.
* `propagate_holes` :  Select whether hole-type charge carriers should be propagated to the electrodes. Defaults to false.
* `output_plots` : Determines if output plots should be generated for every event. This causes a very huge slow down of the simulation, it is not recommended to use this with a run of more than a single event. Disabled by default.
* `output_animation` : In addition to the other output plots, also write a GIF animation of the charges drifting towards the electrodes. This is very slow and writing the animation takes a considerable amount of time.
* `output_plots_step` : Timestep to use between two points that are plotted. Indirectly determines the amount of points plotted. Defaults to *timestep_max* if not explicitly specified.
* `output_plots_theta` : Viewpoint angle of the 3D animation and the 3D line graph around the world X-axis. Defaults to zero.
* `output_plots_phi` : Viewpoint angle of the 3D animation and the 3D line graph around the world Z-axis. Defaults to zero.
* `output_plots_use_pixel_units` : Determines if the plots should use pixels as unit instead of metric length scales. Defaults to false (thus using the metric system).
* `output_plots_use_equal_scaling` : Determines if the plots should be produced with equal distance scales on every axis (also if this implies that some points will fall out of the graph). Defaults to true.
* `output_plots_animation_time_scaling` : Scaling for the animation to use to convert the actual simulation time to the time step in the animation. Defaults to 1.0e9, meaning that every nanosecond is equal to an animation step of a single second.
* `output_plots_contour_max_scaling` : Scaling to use for the contour color axis from the theoretical maximum charge at every single plot step. Default is 10, meaning that the maximum of the color scale axis is equal to the total amount of charges divided by ten (values above this are displayed in the same maximum color). Parameter can be used to improve the color scale of the contour plots.

#### Usage
A example of generic propagation for all Timepix sensors at room temperature using packets of 25 charges is the following:

```
[GenericPropagation]
type = "timepix"
temperature = 293K
charge_per_step = 25
```
