## DefaultDigitizer
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  
**Input**: PixelCharge  
**Output**: PixelHit  

#### Description
Very simple digitization module which translates the collected charges into a digitized signal proportional to the input charge. It simulates noise contributions from the readout electronics as Gaussian noise and allows for a configurable threshold.

In detail, the following steps are performed for every pixel charge:

* A Gaussian noise is added to the input charge value in order to simulate input noise to the preamplifier circuit.
* A charge threshold is applied. Only if the threshold is surpassed, the pixel is accounted for - for all values below the threshold, the pixel charge is discarded. The actually applied threshold is smeared with a Gaussian distribution on an event-by-event basis allowing for simulating fluctuations of the threshold level.
* An inaccuracy of the ADC is simulated using an additional Gaussian smearing, this allows to take ADC noise into account.

With the `output_plots` parameter activated, the module produces histograms of the charge distribution at the different stages of the simulation, i.e. before processing, with electronics noise, after threshold selection, and with ADC smearing applied.
In addition, the distribution of the actually applied threshold is provided as histogram.

#### Parameters
* `electronics_noise` : Standard deviation of the Gaussian noise in the electronics (before applying the threshold). Defaults to 110 electrons.
* `threshold` : Threshold for considering the collected charge as a hit. Defaults to 600 electrons.
* `threshold_smearing` : Standard deviation of the Gaussian uncertainty in the threshold charge value. Defaults to 30 electrons.
* `adc_smearing` : Standard deviation of the Gaussian noise in the ADC conversion (after applying the threshold). Defaults to 300 electrons.
* `output_plots` : Enables output histograms to be be generated from the data in every step (slows down simulation considerably). Disabled by default.

#### Usage
The default configuration is equal to the following:

```ini
[DefaultDigitizer]
electronics_noise = 110e
threshold = 600e
threshold_smearing = 30e
adc_smearing = 300e
```
