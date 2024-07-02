---
# SPDX-FileCopyrightText: 2020-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "CSADigitizer"
description: "Digitizer emulating a Charge Sensitive Amplifier"
module_status: "Functional"
module_maintainers: ["Annika Vauth (<annika.vauth@desy.de>)", "Simon Spannagel (<simon.spannagel@desy.de>)"]
module_inputs: ["PixelCharge"]
module_outputs: ["PixelHit"]
---

## Description

Digitization module which translates the collected charges into a digitized signal, emulating a charge sensitive amplifier with Krummenacher feedback.
For this purpose, a transfer function for a CSA with Krummenacher feedback is taken from \[[@kleczek]\]:

```math
H(s) = \frac{R_f}{(1 + \tau_f s) \cdot (1 + \tau_r s)},
```

with fall time constant

```math
\tau_f = R_f C_f
```

and rise time constant

```math
\tau_r = \frac{C_{det} \cdot C_{out}}{g_m \cdot C_f}
```

The impulse response function of this transfer function is convoluted with the charge pulse. In the time domain, the impulse response function can be written as

```math
\mathcal{L}^{-1}(H) = R_f \left( \frac{e^{-t/\tau_f}}{\tau_f - \tau_r} - \frac{e^{-t/\tau_r}}{\tau_f - \tau_r} \right).
```

This module can be steered by either providing all contributions to the transfer function as parameters within the `csa` model, or using a simplified parametrization providing rise time and feedback time.
In the latter case, the parameters are used to derive the contributions to the transfer function (see e.g. \[[@binkley]\] for calculation of transconductance).

Alternatively a custom impulse response function can be provided by using the `custom` model.

Noise can be applied to the individual bins of the output pulse, drawn from a normal distribution.

The values stored in `PixelHit` depend on the Time-of-Arrival (ToA) and Time-over-Threshold (ToT) settings. If a ToA clock is defined, then `local_time` will be stored in ToA clock cycles, else in time units. If a ToT clock is defined, then `signal` will be the amount of ToT cycles the pulse is above the threshold, else it will be the integral of the amplified pulse.

Since the input pulse may have different polarity, it is important to set the threshold accordingly to a positive or negative value, otherwise it may not trigger at all.
If this behavior is not desired, the `ignore_polarity` parameter can be set to compare only the absolute values of the input and the threshold value.

## Parameters

* `model`: Choice between different CSA models. Currently implemented are two parametrizations of the circuit from \[[@kleczek]\], `simple` and `csa`, the `custom` model for a custom impulse response, and the `graph` model, which takes a .csv file as input and reads out the graph of the transfer function.
* `integration_time`: The length of time the amplifier output is registered. Defaults to 500 ns.
* `sigma_noise`: Standard deviation of the Gaussian-distributed noise added to the output signal. Defaults to 0.1 mV.
* `threshold`: Threshold for TOT/TOA logic, for considering the output signal as a hit. Defaults to 10mV.
* `ignore_polarity`: Select whether polarity of the threshold is ignored, i.e. the absolute values are compared, or if polarity is taken into account. Defaults to `false`.
* `clock_bin_toa`: Duration of a clock cycle for the time-of-arrival (ToA) clock. If set, the output timestamp is delivered in units of ToA clock cycles, otherwise in nanoseconds.
* `clock_bin_tot`: Duration of a clock cycle for the time-over-threshold (ToT) clock. If set, the output charge is delivered as time over threshold in units of ToT clock cycles, otherwise the pulse integral is stored instead.
* `sync_sim_time`: Aligns the clock cycle with global 0 time. Defaults to false.
* `tdc_offset`: Adds an offset to the global time for this digitizer. Defaults to 0ns.

### Parameters for the simplified model

* `feedback_capacitance`: The feedback capacity to the amplifier circuit. Defaults to 5e-15 F.
* `rise_time_constant`: Rise time constant of CSA output. Defaults to 1 ns.
* `feedback_time_constant`: Feedback time constant of CSA output. Defaults to 10 ns.

### Parameters for the CSA model

* `feedback_capacitance`: The feedback capacity to the amplifier circuit. Defaults to 5e-15 F.
* `krummenacher_current`: The feedback current setting of the CSA. Defaults to 20 nA.
* `input_capacitance`: The input capacitance which comprises the capacitance of the detector, the capacitance of the feedback circuit, and any additional capacitance caused by parasitic effects. Defaults to 100 e-15 F.
* `amp_output_capacitance`: The capacitance at the amplifier output. Defaults to 20 e-15 F.
* `transconductance`: The transconductance of the input transistor of the CSA core amplifier. Defaults to 50e-6 C/s/V.
* `weak_inversion_slope`: The weak inversion slope. Defaults to 1.5.
* `temperature`: Defaults to 293.15K.

### Parameters for the custom model

* `response_function`: A 1-dimensional `ROOT::TFormula` \[[@rootformula]\] expression for the impulse response function.
* `response_parameters`: Array of the parameters in the response function. The number of parameters given need to match up with the number of parameters in the formula.

### Parameters for the graph model

* `graph_file`: The path to the .csv file containing the graph of the response function.
* The file should be written in the following format: `x,y` (comma separated values), where x is the time and y the amplitude of the response function at that time point. Each pair of values should be written in a new line.
* `graph_time_unit`: Time unit in which the time on the response function graph is expressed. Should be a double. Defaults to seconds (s).
* `graph_amplitude_unit`: The unit in which data on the y-axis of the response function graph is expressed. Should be a double. Defaults to volts per electron (V/e).

### Plotting parameters

* `output_plots`: Enables simple output histograms to be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale`: Set the x-axis scale of the output histograms, defaults to 30ke.
* `output_plots_bins`: Set the number of bins for the output histograms, defaults to 100.
* `output_pulsegraphs`: Determines if pulse graphs should be generated for every event. This creates several graphs per event, depending on how many pixels see a signal, and can slow down the simulation. It is not recommended to enable this option for runs with more than a couple of events. Disabled by default.

## Usage

Example how to use the `csa` model in this module:

```ini
[CSADigitizer]
model = "csa"
feedback_capacitance = 10e-15C/V
input_capacitance = 100e-15C/V
krummenacher_current = 25e-9C/s
amp_output_capacitance = 15e-15C/V
transconductance = 50e-6C/s/V
weak_inversion_slope = 1.15
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
clock_bin_toa = 1.5625ns
clock_bin_tot = 25.0ns
```

Example for the `custom` model:

```ini
[CSADigitizer]
model = "custom"
response_function = "TMath::Max([0]*(1.-TMath::Exp(-x/[1]))-[2]*x,0.)"
response_parameters = [2.6e14V/C, 9.1e1ns, 4.2e19V/C/s]
integration_time = 10us
threshold = 60mV
clock_bin_toa = 8ns
clock_bin_tot = 8ns
```

Example for the `graph` model:

```ini
[CSADigitizer]
model = "graph"
graph_file = /path/to/response_function.csv
integration_time = 10ns
graph_time_unit = 1s
graph_amplitude_unit = 1.0V/e
```

[@kleczek]: https://doi.org/10.1109/MIXDES.2015.7208529
[@binkley]: https://doi.org/10.1002/9780470033715
[@rootformula]: https://root.cern.ch/doc/master/classTFormula.html
