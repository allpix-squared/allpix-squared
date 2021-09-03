# TransientPropagation
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Status**: Functional  
**Input**: DepositedCharge  
**Output**: PropagatedCharge

### Description
Simulates the transport of electrons and holes through the sensitive sensor volume of the detector. It allows to propagate sets of charge carriers together in order to speed up the simulation while maintaining the required accuracy. The propagation process for these sets is fully independent and no interaction is simulated. The maximum size of the set of propagated charges and thus the accuracy of the propagation can be controlled.

The propagation consists of a combination of drift and diffusion simulation. The drift is calculated using the charge carrier velocity derived from the charge carrier mobility and the magnetic field via a calculation of the Lorentz drift. The mobility model can be chosen using the `mobility_model` parameter, and a list of available models can be found in the user manual.

A fourth-order Runge-Kutta-Fehlberg method [@fehlberg] is used to integrate the particle motion through the electric and magnetic fields. After every Runge-Kutta step, the diffusion is accounted for by applying an offset drawn from a Gaussian distribution calculated from the Einstein relation

$`\sigma = \sqrt{\frac{2k_b T}{e}\mu t}`$

using the carrier mobility $`\mu`$, the temperature $`T`$ and the time step $`t`$. The propagation stops when the set of charges reaches any surface of the sensor.

The charge transport is parameterized in time and the time step each simulation step takes can be configured.
For each step, the induced charge on the neighboring pixel implants is calculated via the Shockley-Ramo theorem [@shockley] [@ramo] by taking the difference in weighting potential between the current position $`x_1`$ and the previous position $`x_0`$ of the charge carrier

$` Q_n^{ind}  = \int_{t_0}^{t_1} I_n^{ind} = q \left( \phi (x_1) - \phi(x_0) \right)`$

and multiplying it with the charge. The resulting pulses are stored for every set of charge carriers individually and need to be combined for each pixel using a transfer module.

The charge carrier lifetime can be simulated using the doping concentration of the sensor. The recombination model is selected via the `recombination_model` parameter, the default value `none` is equivalent to not simulating finite lifetimes. This feature can only be enabled if a doping profile has been loaded for the respective detector using the DopingProfileReader module.
In each step, the doping-dependent charge carrier lifetime is determined, from which a survival probability is calculated.
The survival probability is calculated at each step of the propagation by drawing a random number from an uniform distribution with $`0 \leq r \leq 1`$ and comparing it to the expression $`dt/\tau`$, where $`dt`$ is the time step of the last charge carrier movement.

The module can produces a variety of plots such as total integrated charge plots as well as histograms on the step length and observed potential differences.

### Parameters
* `temperature`: Temperature of the sensitive device, used to estimate the diffusion constant and therefore the strength of the diffusion. Defaults to room temperature (293.15K).
* `mobility_model`: Charge carrier mobility model to be used for the propagation. Defaults to `jacoboni`, a list of available models can be found in the documentation.
* `recombination_model`: Charge carrier lifetime model to be used for the propagation. Defaults to `none`, a list of available models can be found in the documentation. This feature requires a doping concentration to be present for the detector.
* `charge_per_step`: Maximum number of charge carriers to propagate together. Divides the total number of deposited charge carriers at a specific point into sets of this number of charge carriers and a set with the remaining charge carriers. A value of 10 charges per step is used by default if this value is not specified.
* `timestep`: Time step for the Runge-Kutta integration, representing the granularity with which the induced charge is calculated. Default value is 0.01ns.
* `integration_time`: Time within which charge carriers are propagated. After exceeding this time, no further propagation is performed for the respective carriers. Defaults to the LHC bunch crossing time of 25ns.
* `distance`: Maximum distance of electrodes to be considered for current induction, calculated from the electrode the charge carrier under investigation is below. A distance of `1` for example means that the induced current for the closest electrode plus all neighbors is calculated. It should be noted that the time required for simulating a single event depends almost linearly on the number of pixels the induced charge is calculated for. Usually, a 3x3 grid (9 pixels, distance 1) should suffice since the weighting potential at a distance of more than one pixel pitch often is small enough to be neglected while the simulation time is almost tripled for `distance = 2` (5x5 grid, 25 pixels).
* `ignore_magnetic_field`: The magnetic field, if present, is ignored for this module. Defaults to false.
* `output_plots` : Determines if simple output plots should be generated for a monitoring of the simulation flow. Disabled by default.


### Usage
```toml
[TransientPropagation]
temperature = 293K
charge_per_step = 10
output_plots = true
timestep = 0.02ns
```

[@fehlberg]: https://ntrs.nasa.gov/search.jsp?R=19690021375
[@fossum]: https://doi.org/10.1016/0038-1101(76)90022-8
[@fossum-lee]: https://doi.org/10.1016/0038-1101(82)90203-9
[@haug]: https://doi.org/10.1016/0038-1098(78)90646-4
[@shockley]: https://doi.org/10.1063/1.1710367
[@ramo]: https://doi.org/10.1109/JRPROC.1939.228757
