# DepositionPointCharge
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  
**Output**: DepositedCharge, MCParticle

### Description
Module which deposits a defined number of charge carriers at a specific point within the active volume the detector.
The number of charge carriers to be deposited can be specified in the configuration.

Two different source types are available:

* The `point` source deposits charge carriers at a specific point in the sensor, which can be configured via the `position` parameter with three dimensions. The number of charge carriers deposited can be adjusted using the `number_of_charges` parameter.
* The `mip` model allows to deposit charge carriers along a straight line through the sensor, perpendicular to its surface. Charge carriers are deposited linearly along this line with 80 electron-hole pairs per micrometer. The number of steps through the sensor can be configured using the `number_of_steps` parameter, the position can be given in two dimensions via the `position` parameter.

This module supports three different deposition models:

* In the `fixed` model, charge carriers are always deposited at the exact same position, specified via the `position` parameter, in every event of the simulation. This model is mostly interesting for development of new charge transport algorithms, where the initial position of charge carriers should be known exactly.
* In the `scan` model, the position where charge carriers are deposited changes with every event. The scanning positions are distributed such, that the volume of one pixel cell is homogeneously scanned. The total number of positions is taken from the total number of events configured for the simulation. If this number doesn't allow for a full illumination, a warning is printed, suggesting a different number of events. The pixel volume to be scanned is always placed at the center of the active sensor area. The scan model can be used to generate sensor response templates for fast simulations by generating a lookup table from the final simulation results.
* In the `spot` model, charge carriers are deposited in a Gaussian spot around the configured position. The sigma of the Gaussian distribution in all coordinates can be configured via the `spot_size` parameter. Charge carriers are only deposited inside the active sensor volume.

Monte Carlo particles are generated at the respective positions, bearing a particle ID of -1.
All charge carriers are deposited at time zero, i.e. at the beginning of the event.

### Parameters
* `model`: Model according to which charge carriers are deposited. For `fixed`, charge carriers are deposited at a specific point for every event. For `scan`, the point where charge carriers are deposited changes for every event. For `spot`, depositions are smeared around the configured position.
* `number_of_charges`: Number of charges to deposit inside the sensor. Defaults to 1. Only used for `point` source type.
* `number_of_steps`: Number of steps over the full sensor thickness at which charge carriers are deposited. Only used for `mip` source type. Defaults to 100.
* `source_type`: Modeled source type for the deposition of charge carriers. For `point`, charge carriers are deposited at the position given by the `position` parameter. For `mip`, charge carriers are deposited along a line through the full sensor thickness. Defaults to `point`.
* `position`: Position in local coordinates of the sensor, where charge carriers should be deposited. Expects three values for local-x, local-y and local-z position in the sensor volume and defaults to `0um 0um 0um`, i.e. the center of first (lower left) pixel. Only used for the `fixed` and model. When using source type `mip`, providing a 2D position is sufficient since it only uses the x and y coordinates. If used in scan mode, it allows you to shift the origin of each deposited charge by adding this value.
* `spot_size`: Width of the Gaussian distribution used to smear the position in the `spot` model. Only one value is taken and used for all three dimensions.

### Usage

```toml
[DepositionPointCharge]
number_of_steps = 100
position = -10um 10um 0um
model = "fixed"
source_type = "mip"
```
