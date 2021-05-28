# MuPixDigitizer
**Maintainer**: Stephan Lachnit (<stephanlachnit@debian.org>)  
**Status**: Beta  
**Input**: PixelCharge  
**Output**: PixelHit

### Description
Digitizer for MuPix-Type digitizers. These are charge sensitive digitizers, so this module is similar to the CSADigitizer module. However, there are some key differences to the CSADigitizer.

This module always operates with threshold logic in clock cycles. Instead of returning the ToT like the CSADigitzier module, this module returns the clock cycles of the first threshold crossing (TS1) and the last threshold crossing (TS2). These correspond to `local_time` and `signal` in the `PixelHit` class.

Similar to the CSADigitizer module, this module amplifies the pulse for the `integration_time`. In the reference implementation, TS2 is determined by taking the last threshold crossing from above the threshold to below the threshold within `ts2_integration_time`. This second time starts in the first TS2 clock cycle after TS1 was set. If TS1 is so late the second integration time would exceed the integration time, it is limited to match the integration time.

With the `fast_amplification` parameter a faster but less precise amplification method can be chosen. In the fast mode, the pulse shape is ignored, which reduces time precision. This has more impact on the ToA than it has on the ToT (in case of the MuPix10).
Noise can be applied with the `sigma_noise` parameter similar to the CSADigitizer module.

Currently, only the MuPix10 is implented in three modes: the normal single threshold mode, a mode with a second threshold, and a mode with a rising threshold. The amplification curve can be approximated with $`Amp(t,Q) = Q \cdot A (1 - \exp{-\frac{t}{R}} - F \cdot t`$.

The first mode operates similar to the CSADigitzer module. The second mode adds a second, higher threshold that when crossed sets the hit flag. If the second threshold is not crossed, not hit will be registerd. TS1 and TS2 are still set by the first threshold. The ramp mode adds linear rising threshold that gets activated when the pulse hits the first threshold. TS2 is set when the amplified pulse hits this rising threshold.

The module can be easily extend with different model by deriving a class from the `MuPixModel` class, which acts a reference class. Amplification, TS1 and TS2 calculation can be adjusted freely.

### Parameters
* `model` : Choice between different Chips. Currently implemented are `mupix10`, `mupix10double` and `mupix10ramp`.
* `fast_amplification` : Whether to amplify the pulse fast or precise. Defaults to false.
* `sigma_noise` : Standard deviation of the Gaussian-distributed noise added to the output signal. Defaults to 1 mV.
* `threshold` : Threshold for considering the output signal as a hit. Defaults to 30 mV.
* `clock_bin_ts1` : Duration of a clock cycle for the TS1 clock. Defaults to 8 ns.
* `clock_bin_ts2` : Duration of a clock cycle for the TS2 clock. Defaults to 128 ns.
* `integration_time` : Duration for which the charge pulse is amplified. Defaults to 2 us.
* `ts2_integration_time` : Duration in which to look for the last treshold crossing. Defaults to 2 us.

#### Parameters for the MuPix10 (`mupix10`)
* `parameters` : Array containing the impulse response parameters (A, R, F).

#### Parameters for the MuPix10 with second threshold (`mupix10double`)
* `parameters` : Same as for the `mupix10` model.
* `threshold_high` : Second threshold, which when crossed sets the hit flag. Defaults to 40 mV.

#### Parameters for the MuPix10 in ramp mode (`mupix10ramp`)
* `parameters` : Same as for the `mupix10` model.
* `threshold_slew_rate` : Slew rate of the second threshold.

### Plotting parameters
* `output_plots` : Enables simple output histograms to be be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_pulsegraphs`: Determines if pulse graphs should be generated for every event. This creates several graphs per event, depending on how many pixels see a signal, and can slow down the simulation. It is not recommended to enable this option for runs with more than a couple of events. Disabled by default.

### Usage
Example for the MuPix10 with a single threshold
```ini
[MuPixDigitizer]
model = "mupix10"
fast_amplification = true
sigma_noise = 1mV
treshold = 40mV
clock_bin_ts1 = 8ns
clock_bin_ts2 = 128ns
parameters = 2.51424577e+14V/C 3.35573247e-07V/C/s 1.85969061e+04V/s
```
Example for the MuPix10 with a second threshold
```ini
[MuPixDigitizer]
model = "mupix10double"
threshold_high = 40mV
```
Example for the MuPix10 in ramp mode
```ini
[MuPixDigitizer]
model = "mupix10double"
threshold_slew_rate = 1mV/ns
```
