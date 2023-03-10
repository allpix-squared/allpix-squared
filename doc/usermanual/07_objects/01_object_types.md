---
# SPDX-FileCopyrightText: 2022-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Object Types"
weight: 1
---

The list of currently supported objects is given below. A `typedef` is added to every object in order to provide an
alternative name for the message which is directly indicating the carried object.

For writing analysis scripts, a detailed description of the code interface for each object can be found in the
[Object Group](https://allpix-squared.docs.cern.ch/reference/group__Objects.html) of the Doxygen reference manual
\[[@ap2-doxygen]\].

## MCTrack

The MCTrack objects reflects the state of a particle's trajectory when it was created and when it terminates. Moreover, it
allows to retrieve the hierarchy of secondary tracks. This can be done via the parent-child relations the MCTrack objects
store, allowing retrieval of the primary track for a given track. Combining this information with MCParticles allows the
Monte-Carlo trajectory to be fully reconstructed. In addition to these relational information, the MCTrack stores information
on the initial and final point of the trajectory (in *global* coordinates), the initial and final timestamps in global
coordinates of the event, the energies (total as well as kinetic only) at those points, the creation process type, name, and
the volume it took place in. Furthermore, the particle's PDG id is stored.

Main properties:
- Global points where track came into and went out of existence (
  [`getStartPoint()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html#a1bddb8af8c3f64067bf8767c5a435117),
  [`getEndPoint()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html#a1ae55b27c872c7adfa6fd890f2f83195))
- Global time when the track had its first and last appearance (
  [`getGlobalStartTime`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html#a913166027c8a2dec9ffba22361b289f1),
  [`getGlobalEndTime`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html#a01377a4a6f447bc85b92e306da5989c3))
- Initial and final kinetic and total energy (
  [`getKineticEnergyInitial()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html#a9998511439c665d777d65779f9f9dcdf),
  [`getTotalEnergyInitial()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html#a25c481bd56d0c2ab8ba15c4292c8ecc7),
  [`getKineticEnergyFinal()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html#a40419d59795984f813e0611205af6740),
  [`getTotalEnergyFinal()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html#adfb09b0a238968fb728a3668d67f739e))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCTrack.html).

## MCParticle

The Monte-Carlo truth information about the particle passage through the sensor. A start and end point are stored in the
object: for events involving a single MCParticle passing through the sensor, the start and end points correspond to the entry
and exit points. The exact handling of non-linear particle trajectories due to multiple scattering is up to module. In
addition, it provides a member function to retrieve the reference point at the sensor center plane in local coordinates for
convenience. The MCParticle also stores an identifier of the particle type, using the PDG particle codes \[[@pdg]\], as well
as the time it has first been observed in the respective sensor. The MCParticle additionally stores a parent MCParticle
object, if available. The lack of a parent doesn't guarantee that this MCParticle originates from a primary particle, but
only means that no parent on the given detector exists. Also, the MCParticle stores a reference to the MCTrack it is
associated with.

MCParticles provide local and global coordinates in space for both the entry and the exit of the particle in the sensor
volume, as well as local and global time information. The global spatial coordinates are calculated with respect to the
global reference frame defined in [Section 5.1](../05_geometry_detectors/01_geometry.md#coordinate-systems), the global time
is counted from the beginning of the event. Local spatial coordinates are determined by the respective detector, the local
time measurement references the entry point of the *first* MCParticle of the event into the detector.

Main parameters:
- Entry and exit points of the particle in the sensor in local and global coordinates (
  [`getLocalStartPoint()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a3ab0b177b8b64535057d98bd3238cae3),
  [`getGlobalStartPoint()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a2f6a661fe23e0fcc102af99fe044db5a),
  [`getLocalEndPoint()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a7bf3fe84684c26be72cdf2442b986fe8),
  [`getGlobalEndPoint()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a1529985658a12ea1c26bed764dec001d))
- The arrival time of the particle in the sensor in local and global coordinates (
  [`getLocalTime()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a354c07df3e02198e7b2a6d856765d2c5),
  [`getGlobalTime()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#ac50facfceaf33ebdc7199085ec3549f7))
- PDG id for this particle type (
  [`getParticleID()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a940f894b9773e58eed588acd85712bd4))
- If the particle is a primary particle or a secondary particle with a parent (
  [`isPrimary()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a7cc9e9f4ace629928a34c5e3f72d5efa),
  [`getParent()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a8985cb34f41e91cf6f193ac72b9f0ed3))
- The track of the particle (
  [`getTrack()`](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html#a997b124cb9020557ffb8bf18620eb970))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1MCParticle.html).

## DepositedCharge

The set of charge carriers deposited by an ionizing particle crossing the active material of the sensor. The object stores
the *local* position in the sensor together with the total number of deposited charges in elementary charge units. In
addition, the time (in *ns* as the internal framework unit) of the deposition after the start of the event and the type of
carrier (electron or hole) is stored.

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1DepositedCharge.html).

## PropagatedCharge

The set of charge carriers propagated through the sensor due to drift and/or diffusion processes. The object should store the
final *local* position of the propagated charges. This is either on the pixel implant (if the set of charge carriers are
ready to be collected) or on any other position in the sensor if the set of charge carriers got trapped or was lost in
another process. Timing information giving the total time to arrive at the final location, from the start of the event, can
also be stored.

The state of the charge carrier at the end of the propagation can be retrieved via the `getState()` method. The following
values are available:

- `CarrierState::UNKNOWN`:
  The final state of the charge carrier is unknown, for example because it might not have been provided by the used
  propagation algorithm.

- `CarrierState::MOTION`:
  The charge carrier was still in motion when the propagation routine finished, for example when the configured integration
  time was reached.

- `CarrierState::RECOMBINED`:
  The charge carrier has recombined with the silicon lattice at the given position.

- `CarrierState::TRAPPED`:
  The charge carrier has been trapped by a lattice defect at the given position.

- `CarrierState::HALTED`:
  The motion of the charge carrier has stopped, for example because it has reached an implant or the sensor surface.

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1PropagatedCharge.html).

## PixelCharge

The set of charge carriers collected at a single pixel. The pixel indices are stored in both the x and y direction, starting
from zero for the first pixel. Only the total number of charges at the pixel is currently stored, the timing information of
the individual charges can be retrieved from the related `PropagatedCharge` objects.

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1PixelCharge.html).

## PixelHit

The digitised pixel hits after processing in the detector's front-end electronics. The object allows the storage of both the
time and signal value. The time can be stored in an arbitrary unit used to timestamp the hits. The signal can hold different
kinds of information depending on the type of the digitizer used. Examples of the signal information is the "true"
information of a binary readout chip, the number of ADC counts or the ToT (time-over-threshold).

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classallpix_1_1PixelHit.html).


[@pdg]: http://hepdata.cedar.ac.uk/lbl/2016/reviews/rpp2016-rev-monte-carlo-numbering.pdf
[@ap2-doxygen]: https://allpix-squared.docs.cern.ch/reference/
