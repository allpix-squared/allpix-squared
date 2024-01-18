---
# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DefaultDigitizer"
description: "Digitizer that creates a signal proportional to the collected charge"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@desy.de>)"]
module_inputs: ["PixelCharge"]
module_outputs: ["PixelHit"]
---

## Description
Simple digitization module which translates the collected charges into a digitized signal proportional to the input charge. It simulates noise contributions from the readout electronics as Gaussian noise and allows for a configurable threshold. Furthermore, the linear response of an QDC as well as a TDC with configurable resolution can be simulated.
For maximum simplicity only the absolute of the charge is used and compared to a positive threshold.

In detail, the following steps are performed for every pixel charge:

* A Gaussian noise is added to the input charge value in order to simulate input noise to the preamplifier circuit.
* The preamplifier is simulated by applying a gain function to the input charge, or by multiplying the input charge with a defined gain factor.
* An optional simplistic front-end saturation can be simulated which replaces the measured pixel charge with a value drawn from a Gaussian distribution with the configured saturation mean and width if the charge measured is larger than the calculated saturation value. This follows the approach taken in \[[@holmestad]\]. The pixel charge is compared to the smeared saturation value in order to generate a smooth transition rather than an edge in the spectrum.
* A charge threshold is applied. Only if the threshold is surpassed, the pixel is accounted for - for all values below the threshold, the pixel charge is discarded. The actually applied threshold is smeared with a Gaussian distribution on an event-by-event basis allowing for simulating fluctuations of the threshold level. It should be noted that only positive threshold values are possible, and that this threshold will be compared to the absolute of the charge. This therefore both works for positive and negative inputs.
* A charge-to-digital converter (QDC) with configurable resolution, given in bit, can be simulated. For this, first an inaccuracy of the QDC is simulated using an additional Gaussian smearing which allows to take QDC noise into account. Then, the charge is converted into QDC units using the `qdc_slope` and `qdc_offset` parameters provided. Finally, the calculated value is clamped to be contained within the QDC resolution, over- and underflows are treated as saturation.
The QDC implementation also allows to simulate ToT (time-over-threshold) devices by setting the `qdc_offset` parameter to the negative `threshold`. Then, the QDC only converts charge above threshold.
* A time-to-digital converter (TDC) with configurable resolution, given in bit, can be simulated if pulse information is available from the input data. If the necessary pulse information is available from the input data, e.g. by using the PulseTransfer module to generate PixelCharge objects, this module calculates the time-of-arrival (ToA) as the time when the integrated input charge crosses the threshold. Also here, the absolute of the integrated charge is compared to a positive threshold value to be independent of the signal polarity.
First, the time from the start of the event until the first crossing of the charge threshold is calculated. It should be noted that this calculation does not take into account charge noise simulated in the QDC. The resulting ToA is smeared with a Gaussian distribution which allows to take TDC fluctuations into account. Then, the ToA is converted into TDC units using the `tdc_slope` and `tdc_offset` parameters provided. Finally, the calculated value is clamped to be contained within the TDC resolution, over- and underflows are treated as saturation.
If no time information is available from the input data, a local time stamp of 0 is stored.
It should be noted that when using the TDC simulation, the local time stamp of the produced PixelHit object is provided in TDC bins rather than in nanoseconds of the framework-internal units. The global timestamp, however, is always provided in nanoseconds and independent of the TDC settings.

### Gain Function

Apart from a linear gain configured via the `gain` parameter, this module also supports arbitrary gain/response functions, defined via the `gain_function` parameter, which depends on the input charge.
In order to use the input charge in the formula, an `x` has to be placed at the respective position.

Parameters of the function can either be placed directly in the formula in framework-internal units, or provided separately as arrays via the `gain_parameters` parameter.
Placeholders for parameters in the formula are denoted with squared brackets and a parameter number, for example `[0]` for the first parameter provided.
Parameters specified separately from the formula can contain units which will be interpreted automatically - parameters directly placed in the mobility formula have to be supplied in framework-internal units since the function will be evaluated in internal units.
It is recommended to use the possibility of separately configuring the parameters and to make use of units to avoid conversion mistakes.

As an example, the following configuration implements a surrogate response function, i.e.

```math
f(q) = a \cdot q + b - \frac{c}{q - t}
```

```ini
gain_function = "[0]*x + [1] - [2] / (x - [3])"
gain_parameters = 1.09, 5.8e, 130.5e*e, 20.2e
```

### Output Plots

With the `output_plots` parameter activated, the module produces histograms of the charge distribution at the different stages of the simulation, i.e. before processing, with electronics noise, after threshold selection, and with ADC smearing applied.
A 2D-histogram of the actual pixel charge in electrons and the converted charge in QDC units is provided if QDC simulation is enabled by setting `qdc_resolution` to a value different from zero.
In addition, the distribution of the actually applied threshold is provided as histogram.


## Parameters
* `threshold` : Threshold for considering the collected charge as a hit (No default value; required parameter).
* `threshold_smearing` : Standard deviation of the Gaussian uncertainty in the threshold charge value. Defaults to 30 electrons.
* `electronics_noise` : Standard deviation of the Gaussian noise in the electronics (before amplification and application of the threshold). Defaults to 110 electrons.
* `gain` : Gain factor the input charge is multiplied with, defaults to 1.0 (no gain) if no gain function is supplied. `gain` and `gain_function` are mutually exclusive.
* `gain_function` : Formula describing the gain as a function of the input charge. `gain` and `gain_function` are mutually exclusive.
* `gain_parameters` : Parameters of the gain formula. This parameter needs to be provided as array of values, physical units are supported for each parameter individually.
* `saturation`: Enable front-end saturation simulation. Defaults to `false`.
* `saturation_mean`: Mean of the simulated front-end saturation charge, defaults to `190ke`. Only used if `saturation` is `true.`
* `saturation_width`: Width of the Gaussian distribution used to calculate the new charge value of the simulated front-end saturation, defaults to `20ke`. Only used if `saturation` is `true.`
* `qdc_resolution` : Resolution of the QDC in units of bits. Thus, a value of 8 would translate to a QDC range of 0 -- 255. A value of 0bit switches off the QDC simulation and returns the actual charge in electrons. Defaults to 0.
* `qdc_smearing` : Standard deviation of the Gaussian noise in the ADC conversion (after applying the threshold). Defaults to 300 electrons.
* `qdc_slope` : Slope of the QDC calibration in electrons per ADC unit (unit: "e"). Defaults to 10e.
* `qdc_offset` : Offset of the QDC calibration in electrons. In order to simulate a ToT (time-over-threshold) device, this offset should be configured to the negative value of the threshold. Defaults to 0.
* `allow_zero_qdc`: Allows the QDC to return a value of zero if enabled, otherwise the minimum value returned is one. Defaults to `false`. When enabled special care should be taken when analyzing data since charge-weighted cluster position interpolation might return unexpected results.
* `tdc_resolution` : Resolution of the TDC in units of bits. Thus, a value of 8 would translate to a TDC range of 0 -- 255. A value of 0bit switches off the TDC simulation and returns the actual time of arrival in nanoseconds. Defaults to 0.
* `tdc_smearing` : Standard deviation of the Gaussian noise in the TDC conversion. Defaults to 50 ps.
* `tdc_slope` : Slope of the TDC calibration in nanoseconds per TDC unit (unit: "ns"). Defaults to 10ns.
* `tdc_offset` : Offset of the TDC calibration in nanoseconds. Defaults to 0.
* `allow_zero_tdc`: Allows the TDC to return a value of zero if enabled, otherwise the minimum value returned is one. Defaults to `false`.
* `output_plots` : Enables output histograms to be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of charge-related output plot, defaults to 30ke.
* `output_plots_timescale` : Set the x-axis scale of time-related output plot, defaults to 300ns.
* `output_plots_bins` : Set the number of bins for the output plot histograms, defaults to 100.


## Usage
The default configuration is equal to the following:

```ini
[DefaultDigitizer]
electronics_noise = 110e
threshold = 600e
threshold_smearing = 30e
qdc_smearing = 300e
```


[@holmestad]: https://inspirehep.net/literature/1813192
