# MagneticFieldReader
**Maintainer**: Paul Schuetze (<paul.schuetze@desy.de>)
**Status**: Testing

### Description
Adds a magnetic field to the full volume, including the active sensors. By default, the magnetic field is turned off.

The magnetic field reader only provides constant magnetic fields, read in as a three-dimensional vector. The magnetic field is forwarded to the GeometryManager, enabling the magnetic field for the particle propagation via Geant4, as well as to all detectors for enabling a Lorentz drift during the charge propagation.

### Parameters
* `model` : Type of the magnetic field model, currently only **constant**. Default is therefore **constant**.
* `magnetic_field` : Vector describing the magnetic field.

### Usage
A simple example is given below

```ini
[MagneticFieldReader]
model = "constant"
magnetic_field = 500mT 3.8T 0T
```