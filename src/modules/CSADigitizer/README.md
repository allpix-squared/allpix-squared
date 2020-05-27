# CSADigitizer
**Maintainer**: 
**Status**: 
**Input**: PixelCharge  
**Output**: PixelHit  

### Description
Digitization module which translates the collected charges into a digitized signal, emulating a charge sensitive amplifier with Krummenacher feedback.


### Parameters
* `simple_parametrisation` :  Choice between giving only the time constants and feedback capacity, rather than a more detailed parametrisation. Defaults to true.
* `feedback_capacitance` :  The feedback capacity to the amplifier circuit. Defaults to 5e-15 F.
* `rise_time_constant` : Used with simple_parametrisation. Rise time constant of CSA output. Defaults to 1 ns.  
* `feedback_time_constant` : Used with simple_parametrisation. Rise time constant of CSA output. Defaults to 10 ns.
* `krummenacher_current` : Used when simple_parametrisation is not chosen. The feedback current setting of the CSA. Defaults to 20 nA.
* `detector_capacitance` : Used when simple_parametrisation is not chosen. The detector capacitance. Defaults to 100 e-15 F.
* `amp_output_capacitance` : Used when simple_parametrisation is not chosen. The capacitance at the amplifier output. Defaults to 20 e-15 F.
* `transconductance` : Used when simple_parametrisation is not chosen. The transconductance of the CSA feedback circuit. Defaults to 50e-6 C/s/V.
* `v_temperature` : Used when simple_parametrisation is not chosen. From Boltzmann kT . Defaults to 25.7e-3 eV (298K).
* `amp_time_window` : The length of time the amplifier output is registered. Defaults to 500 ns.
* `sigma_noise` : Standard deviation of the Gaussian-distributed noise added to the output signal. Defaults to 0.1 mV.
* `threshold` : Threshold for TOT/TOA logic, for considering the output signal as a hit. Defaults to 10mV.
* `output_plots` : Enables simple output histograms to be be generated from the data in every step (slows down simulation considerably). Disabled by default.
* `output_plots_scale` : Set the x-axis scale of the output histograms, defaults to 30ke.
* `output_plots_bins` : Set the number of bins for the output histograms, defaults to 100.
* `output_pulsegraphs`: Determines if pulse graphs should be generated for every event. This creates several graphs per event, depending on how many pixels see a signal, and can slow down the simulation. It is not recommended to enable this option for runs with more than a couple of events. Disabled by default.


### Usage
The default configuration is equal to the following:

```ini
[CSADigitizer]
feedback_capacitance = 5e-15 C/V
rise_time_constant = 1e-9 s  
feedback_time_constant = 10e-9 s  // R_f * C_f
amp_time_window = 0.5e-6 s
threshold = 10e-3 V
sigma_noise = 0.1e-3 V
```
