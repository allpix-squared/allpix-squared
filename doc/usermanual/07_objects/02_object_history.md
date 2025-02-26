---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Object History"
weight: 2
---

Objects may carry information about the objects which were used to create them. For example, a `PropagatedCharge` could hold
a link to the `DepositedCharge` object at which the propagation started. All objects created during a single simulation event
are accessible until the end of the event; more information on object persistency within the framework can be found in
[Section 4.6](../04_framework/06_messages.md#persistency).

Object history is implemented using the ROOT TRef class \[[@roottref]\], which acts as a special reference. On construction,
every object gets a unique identifier assigned, that can be stored in other linked objects. This identifier can be used to
retrieve the history, even after the objects are written out to ROOT TTrees \[[@roottree]\]. TRef objects are however not
automatically fetched and can only be retrieved if their linked objects are available in memory, which has to be ensured
explicitly. Outside the framework this means that the relevant tree containing the linked objects should be retrieved and
loaded at the same entry as the object that request the history. Whenever the related object is not in memory (either because
it is not available or not fetched) a `MissingReferenceException` will be thrown.

A MCTrack which originated from another MCTrack is linked via a reference to this track, this way the track hierarchy can be
obtained. Every MCParticle is linked to the MCTrack it is associated with. A MCParticle can furthermore be linked to another
MCParticle on the same detector. This will be the case if there are MCParticles from a primary (parent) and secondary (child)
track on one detector. The corresponding child MCParticles will then carry a reference to the parent MCParticle.


[@roottref]: https://root.cern.ch/root/htmldoc/guides/users-guide/InputOutput.html
[@roottree]: https://root.cern.ch/root/htmldoc/guides/users-guide/Trees.html
