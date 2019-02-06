# DepositionPointCharge
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle

### Description
Module which deposits a defined number of charge carriers at a specific point within the active volume the detector.
The number of charge carriers to be deposited can be specified in the configuration.

This module supports two different deposition model:
* In the `point` model, charge carriers are always deposited at the exact same position, specified via the `position` parameter, in every even of the simulation.
* In the `scan` model, the position where charge carriers are deposited changes with every event. The scanning positions are distributed such, that the volume of one pixel cell is homogeneously scanned. The total number of positions is taken from the total number of events set for the simulation. If this number doesn't allow for a full illumination, a warning is printed, suggesting a different number of events. The pixel volume to eb scanned is always placed at the center of the active sensor area.

Monte Carlo particles are generated at the respective positions, bearing a particle ID of -1.
All charge carriers are deposited at time zero, i.e. at the beginning of the event.

### Parameters
* `model`: Model according to which charge carriers are deposited. For `point`, charge carriers are deposited at a specific point for every event. For `scan`, the point where charge carriers are deposited changes for every event.
* `number_of_charges`: Number of charges to deposit inside the sensor. Defaults to 1.
* `position`: Position in local coordinates of the sensor, where charge carriers should be deposited. Expects three values for local-x, local-y and local-z position in the sensor volume and defaults to `0um 0um 0um`, i.e. the center of the sensor. Only used for the `point` model.

### Usage

```toml
[DepositionPointCharge]
number_of_charges = 100
position = -10um 10um 0um
```
