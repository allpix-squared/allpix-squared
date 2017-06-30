# Dummy
* Maintainer: *Simon Spannagel* (*<simon.spannagel@cern.ch>*)
* Status: *Supported*
* Input: [PixelCharge](../../objects/PixelCharge.hpp)
* Output: [PixelHit](../../objects/PixelCharge.hpp)

#### Description
Default digitization module, outputs a digitized signal proportional to the input charge.  Simulates the readout electronics using gaussian smearing and applying a treshold. 

More detailed it applies the following steps:
* Add Gaussian noise to the input charge value to simulate errors in the readout
* Apply a treshold (with a Gaussian treshold border) for hitting the pixel
* Add extra Gaussian smearing to simulate the digitization process

#### Parameters
* `electronics_noise = 110e`: Standard deviation of the Gaussian noise in the electronics (before applying the treshold)
* `treshold = 600e`: Threshold for considering the readout charge as a hit
* `treshold_smearing = 30e`: Standard deviation of the Gaussian uncertainty in the threshold
* `adc_smearing = 300e`: Standard deviation of the Gaussian noise in the adc conversion (after applying the threshold)

#### Usage
The default configuration can be added with the following configuration
```ini
[DefaultDigitizer]
electronics_noise = 110e
treshold = 600e
treshold_smearing = 30e
adc_smearing = 300e
```
