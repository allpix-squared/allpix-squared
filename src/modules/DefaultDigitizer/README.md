## DefaultDigitizer
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  
**Input**: PixelCharge  
**Output**: PixelHit  

#### Description
Default digitization module, outputs a digitized signal proportional to the input charge.  Simulates the readout electronics with gaussian noise and a treshold. 

More detailed it applies the following steps:

* Add Gaussian noise to the input charge value to simulate errors in the readout
* Apply a treshold (with a Gaussian treshold border) for hitting the pixel
* Add extra Gaussian smearing to simulate the digitization process

#### Parameters
* `electronics_noise` : Standard deviation of the Gaussian noise in the electronics (before applying the treshold). Defaults to 110 electrons.
* `treshold` : Threshold for considering the readout charge as a hit. Defaults to 600 electrons.
* `treshold_smearing` : Standard deviation of the Gaussian uncertainty in the threshold. Defaults to 30 electrons.
* `adc_smearing` : Standard deviation of the Gaussian noise in the ADC conversion (after applying the threshold). Defaults to 300 electrons.

#### Usage
The default configuration is equal to the following

```ini
[DefaultDigitizer]
electronics_noise = 110e
treshold = 600e
treshold_smearing = 30e
adc_smearing = 300e
```
