# DepositionReader
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)
**Status**: Immature
**Output**: DepositedCharge

### Description
*Short description of this module*

#### CSV Files

Data in CSV-formatted text files are interpreted as follows.
Empty lines as well as lines starting with a hash (`#`) are ignored, all other lines are interpreted either as event header if they start with `E`, or as energy deposition:

```csv
Event: <N>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>
# ...
# For example:
211, 3.234674e+01, 1.091620e-02, -2.515335e+00, 4.427924e+00, -2.497500e-01, MyDetector

Event: <N+1>
<PID>,<T>,<E>,<X>,<Y>,<Z>,<V>
# ...
```

where `<N>` is the current event number, `<PID>` is the PDG particle ID [@pdg], `<T>` the time of deposition, calculated from the beginning of the event, `<E>` is the deposited energy in units of `keV`m `<[X-Z]>` is the position of the energy deposit in global coordinates of the setup, and `<V>` the the detector name (volume) the energy deposit should be assigned to.

Only energy deposits with a valid volume are considered, i.e. where a matching detector with the same name can be found in the geometry setup.
The global coordinates are then translated to local coordinates of the given detector.
If these are outside the sensor, the energy deposit is discarded.


### Parameters
* `model`: Format of the data file to be read, can either be `csv` or `root`.

### Usage
*Example how to use this module*

[@pdg]: http://hepdata.cedar.ac.uk/lbl/2016/reviews/rpp2016-rev-monte-carlo-numbering.pdf
