## ProjectionPropagation
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>), Paul Schuetze (<paul.schuetze@desy.de>)  
**Status**: Review   
**Input**: DepositedCharge   
**Output**: PropagatedCharge   

#### Description
The module projects the deposited electrons (holes are ignored) to the sensor surface and applies a randomized diffusion. It can be used as a replacement for a charge propagation (e.g. the GenericPropagation module) for saving computing time at the cost of precision.

The diffusion of the charge carriers is realized by placing sets of a configurable number of electrons in positions drawn as a random number from a two-dimensional gaussian distribution around the projected position at the sensor surface. The diffusion width is based on an approximation of the drift time, using the numerical integral of the mobility in a linear electric field. Here, the `electron_Beta` is set to 1, inducing systematic errors of <10%, depending on the set sensor temperature.

Since the approximation of the drift time assumes a linear electric field, this module cannot be used with any other electric field configuration.

#### Parameters
* `temperature`: Temperature in the sensitive device, used to estimate the diffusion constant and therefore the width of the diffusion distribution.
* `charge_per_step`: Maximum number of electrons placed for which the randomized diffusion is calculated together, i.e. they are placed at the same position. Defaults to 10.
* `output_plots`: Determines if plots should be generated.


#### Usage
```
[ProjectionPropagation]
temperature = 293K
charge_per_step = 10
output_plots = 1
```