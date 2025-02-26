---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Development"
weight: 5
---

#### How do I write my own output module?

An essential requirement of any output module is its ability to receive any message of the framework. This can be implemented
by defining a private `filter` function for the module as described in [Section 4.6](../04_framework/06_messages.md). This
function will be called for every new message dispatched within the framework, and should contain code to decide whether to
discard or cache a message for processing. Heavy-duty tasks such as handling data should not be performed in the `filter`
routine, but deferred to the `run` function of the respective output module. The `filter` function should only decide whether
to keep a message for processing or to discard it before the `run` function.

#### How do I process data from multiple detectors?

When developing a new Allpix Squared module which processes data from multiple detectors, e.g. as the simulation of a track
trigger module, this module has to be of type *unique* as described in [Section 4.4](../04_framework/04_modules.md). As a
*detector* module, it would always only have access to the information linked to the specific detector is has been
instantiated for. The module should then request all messages of the desired type using the messenger call `bindMulti` as
described in [Section 4.6](../04_framework/06_messages.md). For PixelHit messages, an example code would be:

```cpp
TrackTriggerModule(Configuration&, Messenger* messenger, GeometryManager* geo_manager) {
    messenger->bindMulti<MCTrackMessage>(this, MsgFlags::NONE);
}
std::vector<std::shared_ptr<PixelHitMessage>> messages;
```

The correct detectors have then to be selected in the `run` function of the module implementation.

#### How do I calculate an efficiency in a module?

Calculating efficiencies always requires a reference. For hit detection efficiencies in Allpix Squared, this could be the
Monte Carlo truth information available via the MCParticle objects. Since the framework only runs modules, if all input
message requirements are satisfied, the message flags described in
[Section 4.6](../04_framework/06_messages.md#message-flags) have to be set up accordingly. For the hit efficiency example,
two different message types are required, and the Monte Carlo truth should always be required (using `MsgFlags::REQUIRED`)
while the PixelHit message should be optional:

```cpp
MyModule::MyModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {

    // Bind messages
    messenger->bindSingle<PixelHitMessage>(this);
    messenger->bindSingle<MCParticleMessage>(this, MsgFlags::REQUIRED);
}
```

#### How do I add a new sensor material?

When adding a new sensor material, additions at several positions in the code are necessary:

- Add material to list of available sensor materials in `src/core/geometry/DetectorModel.hpp`.

- If not available yet, add material to the Geant4 material manager
  (`src/modules/GeometryBuilderGeant4/MaterialManager.cpp`). See examples of either using a material known to Geant4 or
  defining compositions in the code. It should be noted that the key of the `materials_` map needs to match the name of the
  sensor material defined in the previous step, transformed to lower case letters.

- Define default values for the material properties listed in `src/physics/MaterialProperties.hpp`.

- Add the list of material properties to the corresponding section of the user manual
  (`doc/usermanual/chapters/06_models/01_material_properties.md`).

Any contribution to the framework in terms of new sensor material definitions is welcome and can be added via a dedicated
merge request in the repository \[[@ap2-repo]\].


[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
