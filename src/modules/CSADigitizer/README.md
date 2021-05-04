# CSADigitizer
**Maintainer**: Annika Vauth (<annika.vauth@desy.de>), Simon Spannagel (<simon.spannagel@desy.de>)  
**Status**: Functional  
**Input**: PixelCharge  
**Output**: PixelHit

### Description
Digitization module which translates the collected charges into a digitized signal, emulating a charge sensitive amplifier.

In this module are two main types of amplifiers implemented: one with Krummenacher feedback and one with a linear falling slope like in the MuPix10.

For the amplifiers with Krummenacher feedback, a transfer function for a CSA is taken from [@kleczek] :
$`H(s) = \frac{R_f}{((1+ \tau_f s) * (1 + \tau_r s))}, `$
with $`\tau_f = R_f C_f `$ , rise time constant $`\tau_r = \frac{C_{det} * C_{out}}{g_m * C_f} `$

The impulse response function of this transfer function is convoluted with the charge pulse.
This module can be steered by either providing all contributions to the transfer function as parameters within the `krummenacher` model, or using a simplified parametrization providing rise time and feedback time.
In the latter case, the parameters are used to derive the contributions to the transfer function (see e.g. [@binkley] for calculation of transconductance).

The MuPix10 amplifier provides a charge dependent feedback that creates a linear falling slope. This can be modeled with $`Q * A (1 - \exp{- \frac{t}{R}}) - F * t`$.

Noise can be applied to the individual bins of the output pulse, drawn from a normal distribution.

The values stored in `PixelHit` depend on the Time-of-Arrival (ToA) and Time-over-Threshold (ToT) settings. If a ToA clock is defined, then `local_time` will be stored in ToA clock cycles, else in time units. If a ToT clock is defined, then `signal` will be the amount of ToT cycles the pulse is above the threshold, else it will be the integral of the amplified pulse. 

Since the input pulse may have different polarity, it is important to set the threshold accordingly to a positive or negative value, otherwise it may not trigger at all.
If this behavior is not desired, the `ignore_polarity` parameter can be set to compare only the absolute values of the input and the threshold value.


### Parameters
* `model` : Choice between different CSA models. Currently implemented are two parametrizations of the circuit from [@kleczek], `simple` and `csa`, and the `custom` model for a custom impulse response.
* `integration_time` : The length of time the amplifier output is registered. Defaults to 500 ns.
* `sigma_noise` : Standard deviation of the Gaussian-distributed noise added to the output signal. Defaults to 0.1 mV.
* `threshold` : Threshold for TOT/TOA logic, for considering the output signal as a hit. Defaults to 10mV.
* `ignore_polarity`: Select whether polarity of the threshold is ignored, i.e. the absolute values are compared, or if polarity is taken into account. Defaults to `false`.
* `clock_bin_ts1` : Duration of a TS1 clock cycle. This clock is responsible to determine the time-of-arrival (ToA). If set, the output timestamp is delivered in units of TS1 clock cycles, otherwise in nanoseconds.
* `clock_bin_ts2` : Duration of a TS2 clock cycle. This clock is responsible to determine the time-over-threshold (ToT). If set, the output charge is delivered as time over threshold in units of TS2 clock cycles, otherwise the pulse integral is stored instead.
* `signal_is_ts2` : Select whether the second timestamp (TS2) or the ToT is stored. Defaults to `false`.

#### Parameters for the simple model
* `impulse_response_timestep` : Timestep used for the impulse response function. Defaults to 0.01 ns.
* `feedback_capacitance` : The feedback capacity to the amplifier circuit. Defaults to 5e-15 F.
* `rise_time_constant` : Rise time constant of CSA output. Defaults to 1 ns.
* `feedback_time_constant` : Feedback time constant of CSA output. Defaults to 10 ns.

#### Parameters for the krummenacher model
* `impulse_response_timestep` : Timestep used for the impulse response function. Defaults to 0.01 ns.
* `feedback_capacitance` : The feedback capacity to the amplifier circuit. Defaults to 5e-15 F.
* `krummenacher_current` : The feedback current setting of the CSA. Defaults to 20 nA.
* `detector_capacitance` : The detector capacitance. Defaults to 100 e-15 F.
* `amp_output_capacitance` : The capacitance at the amplifier output. Defaults to 20 e-15 F.
* `transconductance` : The transconductance of the CSA feedback circuit. Defaults to 50e-6 C/s/V.
* `temperature` : Defaults to 293.15K.

#### Parameters for the mupix10 model
* `parameter_amplification` : Amplification parameter. Defaults to 2.51424577e+14 V/C.
* `parameter_rise` : Rise parameter. Defaults to 3.35573247e-07 s.
* `parameter_fall` : Fall parameter. Defaults to 1.85969061e+04 V/s.

### Plotting parameters
* `output_plots` : Enables simple output histograms to be be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output histograms, defaults to 30ke.
* `output_plots_bins` : Set the number of bins for the output histograms, defaults to 100.
* `output_pulsegraphs`: Determines if pulse graphs should be generated for every event. This creates several graphs per event, depending on how many pixels see a signal, and can slow down the simulation. It is not recommended to enable this option for runs with more than a couple of events. Disabled by default.


### Usage
Example how to use the `krummenacher` model in this module:
```ini
[CSADigitizer]
model = "krummenacher"
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

Example for the `simple` model:
```ini
[CSADigitizer]
model = "simple"
feedback_capacitance = 5e-15C/V
rise_time_constant = 1e-9s
feedback_time_constant = 10e-9 s
integration_time = 0.5e-6s
threshold = 10e-3V
clock_bin_ts1 = 1.5625ns
clock_bin_ts2 = 25.0ns
```

Example for the `mupix10` model:
```ini
[CSADigitizer]
model = "mupix10"
parameter_amplification = 2.51424577e+14 V/C
parameter_rise = 3.35573247e-07 s
parameter_fall = 1.85969061e+04 V/s
integration_time = 5us
threshold = 40e-3V
clock_bin_ts1 = 8ns
clock_bin_ts2 = 128ns
signal_is_ts2 = true
```

[@kleczek]:  https://doi.org/10.1109/MIXDES.2015.7208529
[@binkley]: https://doi.org/10.1002/9780470033715
