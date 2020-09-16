# CSADigitizer
**Maintainer**: Annika Vauth (<annika.vauth@desy.de>)
**Status**: Functional
**Input**: PixelCharge
**Output**: PixelHit

### Description
Digitization module which translates the collected charges into a digitized signal, emulating a charge sensitive amplifier with Krummenacher feedback.
For this purpose, a transfer function for a CSA with Krummenacher feedback is taken from [@kleczek] :
$`H(s) = \frac{R_f}{((1+ \tau_f s) * (1 + \tau_r s))}, `$
with $`\tau_f = R_f C_f `$ , rise time constant $`\tau_r = \frac{C_{det} * C_{out}}{g_m * C_f} `$

The impulse response function of this transfer function is convoluted with the charge pulse.
This module can be steered by either providing all contributions to the transfer function as parameters within the `csa` model, or using a simplified parametrisation providing rise time and feedback time.
In the latter case, the parameters are used to derive the contributions to the transfer function (see e.g. [@binkley] for calculation of transconductance).

Noise can be applied to the individual bins of the output pulse, drawn from a normal distribution.
For the amplified pulse signal, alongside the Time-of-Arrival either the determined Time-over-Threshold, or the integral of the amplified pulse can be stored in the `PixelHit`.

Since the input pulse may have different polarity, it is important to set the threshold accordingly to a positive or negative value, otherwise it may not trigger at all.
If this behavior is not desired, the 'Ignore_Polarity' parameter can be set to compare only the absolute values of the input and the threshold value.


### Parameters
* `model` :  Choice between different CSA models. Currently implemented are two parametrisations of the circuit from [@kleczek], `simple` and `csa`.
* `feedback_capacitance` :  The feedback capacity to the amplifier circuit. Defaults to 5e-15 F.
* `integration_time` : The length of time the amplifier output is registered. Defaults to 500 ns.
* `sigma_noise` : Standard deviation of the Gaussian-distributed noise added to the output signal. Defaults to 0.1 mV.
* `threshold` : Threshold for TOT/TOA logic, for considering the output signal as a hit. Defaults to 10mV.
* `ignore_polarity`: Select whether polarity of the threshold is ignored, i.e. the absolute values are compared, or if polarity is taken into account. Defaults to `false`.
* `clock_bin_toa` : Duration of a clock cycle for the time-of-arrival (ToA) clock. If set, the output timestamp is delivered in units of ToA clock cycles, otherwise in nanoseconds.
* `clock_bin_tot` : Duration of a clock cycle for the time-over-threshold (ToT) clock. If set, the output charge is delivered as time over threshold in units of ToT clock cycles, otherwise the pulse integral is stored instead.

#### Parameters for the simplified model
* `rise_time_constant` : Rise time constant of CSA output. Defaults to 1 ns.
* `feedback_time_constant` : Feedback time constant of CSA output. Defaults to 10 ns.

#### Parameters for the CSA model
* `krummenacher_current` : The feedback current setting of the CSA. Defaults to 20 nA.
* `detector_capacitance` : The detector capacitance. Defaults to 100 e-15 F.
* `amp_output_capacitance` : The capacitance at the amplifier output. Defaults to 20 e-15 F.
* `transconductance` : The transconductance of the CSA feedback circuit. Defaults to 50e-6 C/s/V.
* `temperature` : Defaults to 293.15K.

### Plotting parameters
* `output_plots` : Enables simple output histograms to be be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output histograms, defaults to 30ke.
* `output_plots_bins` : Set the number of bins for the output histograms, defaults to 100.
* `output_pulsegraphs`: Determines if pulse graphs should be generated for every event. This creates several graphs per event, depending on how many pixels see a signal, and can slow down the simulation. It is not recommended to enable this option for runs with more than a couple of events. Disabled by default.


### Usage
Two examples how to use this module:

```ini
[CSADigitizer]
model = "csa"
feedback_capacitance = 10e-15C/V
detector_capacitance = 100e-15C/V
krummenacher_current = 25e-9C/s
amp_output_capacitance = 15e-15C/V
transconductance = 50e-6C/s/V
temperature = 298
integration_time = 0.5e-6s
threshold = 10e-3V
sigma_noise = 0.1e-3V
```



```ini
[CSADigitizer]
model = "simple"
feedback_capacitance = 5e-15C/V
rise_time_constant = 1e-9s
feedback_time_constant = 10e-9 s
integration_time = 0.5e-6s
threshold = 10e-3V
clock_bin_toa = 1.5625ns
clock_bin_tot = 25.0ns
```



[@kleczek]:  https://doi.org/10.1109/MIXDES.2015.7208529
[@binkley]: https://doi.org/10.1002/9780470033715.index
