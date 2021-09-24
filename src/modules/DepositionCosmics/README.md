# DepositionCosmics
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Immature  
**Output**: DepositedCharge, MCParticle, MCTrack

### Description
*Short description of this module*

The coordinate system for this module defines the `z` axis orthogonal to the earth surface, pointing towards the earth core.

The are on which incident particles will be simulated is automatically inferred from the total setup size, and the next larger set of tabulated data available is selected.
Data are tabulated for areas of 1m, 3m, 10m, 30m, 100m, and 300m. Particles outside the selected window are dropped.


### Dependencies

This module inherits from and therefore requires the *DepositionGeant4* module as well as an installation Geant4.

### Parameters
* `data_path`: Directory to read the tabulated input data for the CRY framework from. By default, this is the standard installation path of the data files shipped with the framework.

### CRY Framework Parameters
* `return_neutrons`: Boolean to select whether neutrons should be returned to Geant4. Defaults to `true`.
* `return_protons`: Boolean to select whether protons should be returned to Geant4. Defaults to `true`.
* `return_gammas`: Boolean to select whether gammas should be returned to Geant4. Defaults to `true`.
* `return_electrons`: Boolean to select whether electrons should be returned to Geant4. Defaults to `true`.
* `return_muons`: Boolean to select whether muons should be returned to Geant4. Defaults to `true`.
* `return_pions`: Boolean to select whether pions should be returned to Geant4. Defaults to `true`.
* `return_kaons`: Boolean to select whether kaons should be returned to Geant4. Defaults to `true`.
* `altitude`: Altitude for which the shower particles should be simulated. Possible values are `0m`, `2100m` and `11300m`, defaults to ground level, i.e. `0m`.
* `min_particles`: Minimum number of particles required for a shower to be considered.
* `max_particles`: Maximum number of particles in a shower before additional particles are cut off.

### Usage
*Example how to use this module*
