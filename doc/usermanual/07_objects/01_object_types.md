---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Object Types"
weight: 1
---

The list of currently supported objects is given below. A `typedef` is added to every object in order to provide an
alternative name for the message which is directly indicating the carried object.

For writing analysis scripts, a detailed description of the code interface for each object can be found in the
[Object Group](https://allpix-squared.docs.cern.ch/reference/classes/) of the Doxygen reference manual
\[[@ap2-doxygen]\].

## MCTrack

The MCTrack objects reflects the state of a particle's trajectory when it was created and when it terminates. Moreover, it
allows to retrieve the hierarchy of secondary tracks. This can be done via the parent-child relations the MCTrack objects
store, allowing retrieval of the primary track for a given track. Combining this information with [MCParticles](#mcparticle)
allows the Monte-Carlo trajectory to be fully reconstructed. In addition to these relational information, the MCTrack stores
information on the initial and final point of the trajectory (in *global* coordinates), the initial and final timestamps in
global coordinates of the event, the energies (total as well as kinetic only) at those points, the creation process type,
name, and the volume it took place in. Furthermore, the particle's PDG id \[[@pdg]\] is stored.

Main properties:

- Global points where track came into and went out of existence
  ([`getStartPoint()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-getstartpoint),
   [`getEndPoint()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-getendpoint))

- Global time when the track had its first and last appearance
  ([`getGlobalStartTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-getglobalstarttime),
   [`getGlobalEndTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-getglobalendtime))

- Initial and final kinetic and total energy
  ([`getKineticEnergyInitial()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-getkineticenergyinitial),
   [`getTotalEnergyInitial()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-gettotalenergyinitial),
   [`getKineticEnergyFinal()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-getkineticenergyfinal),
   [`getTotalEnergyFinal()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-gettotalenergyfinal))

- Initial and final mometum directions
  ([`getMomentumDirectionInitial()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-getmomentumdirectioninitial),
   [`getMomentumDirectionFinal()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/#function-getmomentumdirectionfinal))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mctrack/).

## MCParticle

The Monte-Carlo truth information about the particle passage through the sensor. A start and end point are stored in the
object: for events involving a single MCParticle passing through the sensor, the start and end points correspond to the entry
and exit points. The exact handling of non-linear particle trajectories due to multiple scattering is up to module. In
addition, it provides a member function to retrieve the reference point at the sensor center plane in local coordinates for
convenience. The MCParticle also stores an identifier of the particle type, using the PDG particle codes \[[@pdg]\], as well
as the time it has first been observed in the respective sensor. The MCParticle additionally stores a parent MCParticle
object, if available. The lack of a parent doesn't guarantee that this MCParticle originates from a primary particle, but
only means that no parent on the given detector exists. Also, the MCParticle stores a reference to the [MCTrack](#mctrack) it
is associated with.

MCParticles provide local and global coordinates in space for both the entry and the exit of the particle in the sensor
volume, as well as local and global time information. The global spatial coordinates are calculated with respect to the
global reference frame defined in [Section 5.1](../05_geometry_detectors/01_geometry.md#coordinate-systems), the global time
is counted from the beginning of the event. Local spatial coordinates are determined by the respective detector, the local
time measurement references the entry point of the *first* MCParticle of the event into the detector.

Main parameters:

- Entry and exit points of the particle in the sensor in local and global coordinates
  ([`getLocalStartPoint()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getlocalstartpoint),
   [`getGlobalStartPoint()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getglobalstartpoint),
   [`getLocalEndPoint()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getlocalendpoint),
   [`getGlobalEndPoint()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getglobalendpoint))

- The arrival time of the particle in the sensor in local and global coordinates
  ([`getLocalTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getlocaltime),
   [`getGlobalTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getglobaltime))

- PDG id for this particle type
  ([`getParticleID()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getparticleid))

- If the particle is a primary particle or has a parent particle
  ([`isPrimary()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getprimary)),
  and the parent particle, if any
  ([`getParent()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-getparent))

- The track the particle is related to, if any
  ([`getTrack()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/#function-gettrack))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1mcparticle/).

## SensorCharge

This is a meta class for a set of charges within a sensor. The properties of this object are inherited by
[DepositedCharge](#depositedcharge) and [PropagatedCharge](#propagatedcharge) objects.

Main parameters:

- The position of the set of charges in the sensor in local and global coordinates
  ([`getLocalPosition()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1sensorcharge/#function-getlocalposition),
   [`getGlobalPosition()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1sensorcharge/#function-getglobalposition))

- The associated time of the set of charges in local and global coordinates
  ([`getLocalTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1sensorcharge/#function-getlocaltime),
   [`getGlobalTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1sensorcharge/#function-getglobaltime))

- The sign of the charge carries
  ([`getSign()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1sensorcharge/#function-getsign))
  and the amount of charges in the set
  ([`getCharge()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1sensorcharge/#function-getcharge))

- The carrier type to check if the charge carriers are electrons or holes
  ([`getType()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1sensorcharge/#function-gettype))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1sensorcharge/).

## DepositedCharge

The set of charge carriers deposited by an ionizing particle crossing the active material of the sensor. The object stores
the *local* position in the sensor together with the total number of deposited charges in elementary charge units. In
addition, the time (in *ns* as the internal framework unit) of the deposition after the start of the event and the type of
carrier (electron or hole) is stored.

Main parameters:

- Everything from [SensorCharge](#sensorcharge)

- The [MCParticle](#mcparticle) that created the deposited charge
  ([`getMCParticle()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1depositedcharge/#function-getmcparticle))

{{% alert title="Warning" color="warning" %}}
It should be noted that in most cases, storage of DepositedCharge objects is not required. Since individual objects are
generated for every electron and hole in the event, storing them will lead to large output files and possible performance penalties.
{{% /alert %}}

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1depositedcharge/).

## PropagatedCharge

The set of charge carriers propagated through the sensor due to drift and/or diffusion processes. The object should store the
final *local* position of the propagated charges. This is either on the pixel implant (if the set of charge carriers are
ready to be collected) or on any other position in the sensor if the set of charge carriers got trapped or was lost in
another process. Timing information giving the total time to arrive at the final location, from the start of the event, can
also be stored.

Main parameters:

- Everything from [SensorCharge](#sensorcharge)

- The associated [DepositedCharge](#depositedcharge) object
  ([`getDepositedCharge()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1propagatedcharge/#function-getdepositedcharge))

- The associated induced [pulses](#pulse), if any
  ([`getPulses()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1propagatedcharge/#function-getpulses))

- The carrier state of the charge carriers described below
  ([`getState()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1propagatedcharge/#function-getstate))

The following values for the carrier state are possible:

- `CarrierState::UNKNOWN`:
  The final state of the charge carrier is unknown, for example because it might not have been provided by the used
  propagation algorithm

- `CarrierState::MOTION`:
  The charge carrier was still in motion when the propagation routine finished, for example when the configured integration
  time was reached

- `CarrierState::RECOMBINED`:
  The charge carrier has recombined with the silicon lattice at the given position

- `CarrierState::TRAPPED`:
  The charge carrier has been trapped by a lattice defect at the given position

- `CarrierState::HALTED`:
  The motion of the charge carrier has stopped, for example because it has reached an implant or the sensor surface

{{% alert title="Warning" color="warning" %}}
It should be noted that in most cases, storage of PropagatedCharge objects is not required. Since individual objects are
generated for every electron and hole in the event, storing them will lead to large output files and possible performance penalties.
{{% /alert %}}

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1propagatedcharge/).

## PixelCharge

The set of charge carriers collected at a single pixel. The pixel indices are stored in both the x and y direction, starting
from zero for the first pixel. Only the total number of charges at the pixel is currently stored, the timing information of
the individual charges can be retrieved from the related [PropagatedCharge](#propagatedcharge) objects.

Main parameters:

- The pixel corresponding to the charge
  ([`getPixel()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/#function-getpixel))
  and its index
  ([`getIndex()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/#function-getindex))

- The charge collected in the pixel
  ([`getCharge()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/#function-getcharge),
   [`getAbsoluteCharge()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/#function-getabsolutecharge))

- The related [propagates charges](#propagatedcharge)
  ([`getPropagatedCharges()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/#function-getpropagatedcharges))

- The associated time of the charge in local and global coordinates
  ([`getLocalTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/#function-getlocaltime),
   [`getGlobalTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/#function-getglobaltime))

- The recorded [charge pulse](#pulse), if any
  ([`getPulse()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/#function-getpulse))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelcharge/).

## Pulse

The pulse object is a meta class mainly used to hold the time information of a charge pulse arriving at the collection
implant, if such information is available in the simulation. A pulse object always has a fixed time binning chosen during the
creation of the object. It inherits from [`std::vector<double>`](https://en.cppreference.com/w/cpp/container/vector).

Main parameters:

- The total charge of the pulse
  ([`getCharge()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pulse/#function-getcharge))

- The time binning of the pulse
  ([`getBinning()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pulse/#function-getbinning))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pulse/)

## PixelHit

The digitised pixel hits after processing the [PixelCharge](#pixelcharge) in the detector's front-end electronics. The object
allows the storage of both the time and signal value. The time can be stored in an arbitrary unit used to timestamp the hits.
The signal can hold different kinds of information depending on the type of the digitizer used. Examples of the signal
information is the "true" information of a binary readout chip, the number of ADC counts or the ToT (time-over-threshold).

The exact type of the time and signal values depends on the digitizer module used, for which the
[module documentation](../08_modules/_index.md) is to be consulted.

Main parameters:

- The pixel corresponding to the hit
  ([`getPixel()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelhit/#function-getpixel))
  and its index
  ([`getIndex()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelhit/#function-getindex))

- The related [PixelCharge](#pixelcharge)
  ([`getPixelCharge()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelhit/#function-getpixelcharge))
  and PixelPulse, if any
  ([`getPixelPulse()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelhit/#function-getpixelpulse))

- The signal of the hit
  ([`getSignal()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelhit/#function-getsignal))

- The time information of the hit in local and global coordinates
  ([`getLocalTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelhit/#function-getlocaltime),
   [`getGlobalTime()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelhit/#function-getglobaltime))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelhit/).

## PixelPulse

If the detector's front-end electronics provide a digitized front-end pulse, this object can be used to store that
information. It inherits from the [Pulse](#pulse) object.

Main parameters:

- Everything from [Pulse](#pulse)

- The pixel corresponding to the digitized pulse
  ([`getPixel()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelpulse/#function-getpixel))
  and its index
  ([`getIndex()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelpulse/#function-getindex))

- The corresponding [pixel charge](#pixelcharge)
  ([`getPixelCharge()`](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelpulse/#function-getpixelcharge))

For more details refer to the [code reference](https://allpix-squared.docs.cern.ch/reference/classes/classallpix_1_1pixelpulse/).


[@pdg]: http://hepdata.cedar.ac.uk/lbl/2016/reviews/rpp2016-rev-monte-carlo-numbering.pdf
[@ap2-doxygen]: https://allpix-squared.docs.cern.ch/reference/
