# DepositionPointCharge
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Immature  
**Output**: DepositedCharge, MCParticle  

### Description
Module which deposits a defined number of charge carriers at a specific point within the active volume of a given detector.
The number of charge carriers as well as the position in local coordinates can be specified in the configuration.
Monte Carlo particles are generated at the respective position, bearing a particle ID of -1.
All charge carriers are deposited at time zero, i.e. at the beginning of the event.

### Parameters
* `number_of_charges`: Number of charges to deposit inside the sensor. Defaults to 1.
* `position`: Position in local coordinates of the sensor, where charge carriers should be deposited. Expects three values for local-x, local-y and local-z position in the sensor volume and defaults to `0um 0um 0um`, i.e. the center of the sensor.

### Usage

```toml
[DepositionPointCharge]
number_of_charges = 100
position = -10um 10um 0um
```
