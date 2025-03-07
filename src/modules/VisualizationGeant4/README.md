---
# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "VisualizationGeant4"
description: "Constructs a Geant4 viewer to display the geometry"
module_status: "Functional"
module_maintainers: ["Simon Spannagel (<simon.spannagel@cern.ch>)"]
---

## Description

Constructs a viewer to display the constructed Geant4 geometry. The module supports all type of viewers included in Geant4, but the default Qt visualization with the OpenGL viewer is recommended as long as the installed Geant4 version supports it.
It offers the best visualization experience.

The module allows for changing a variety of parameters to control the output visualization both for the different detector components and the particle beam.

Both detectors and passive materials will be displayed.
If the material of a passive material is the same as the material of its `mother_volume`, the passive material will not be shown in the visualization. In the case that the material is the same as the material of the world frame, the material will have a white color instead of the default blue in the visualization.

This module does not support multithreading and will force the simulation chain to be executed on a single thread when activated.

## Dependencies

This module requires an installation of Geant4.

## Parameters

* `mode` : Determines the mode of visualization. Options are **gui** which starts a Qt visualization window containing the driver (as long as the chosen driver supports it), **terminal** starts both the visualization viewer and a Geant4 terminal or **none** which only starts the driver itself (and directly closes it if the driver is asynchronous). Defaults to **gui**.
* `driver` : Geant4 driver used to visualize the geometry. All the supported options can be found online \[[@g4drivers]\] and depend on the build options of the Geant4 version used. The default **OGL** should normally be used with the **gui** option if the visualization should be accumulated, otherwise **terminal** is the better option. Other than this, only the **VRML2FILE** driver has been tested. This driver should be used with *mode* equal to **none**. Defaults to the OpenGL driver **OGL**.
* `accumulate` : Determines if all events should be accumulated and displayed at the end, or if only the last event should be kept and directly visualized (if the driver supports it). Defaults to true, thus accumulating events and only displaying the final result.
* `accumulate_time_step` : Time step to sleep between events to allow for time to display if events are not accumulated. Only used if *accumulate* is disabled. Default value is 100ms.
* `simple_view` : Determines if the visualization should be simplified, not displaying the pixel matrix and other parts which are replicated multiple times. Default value is true. This parameter should normally not be changed as it will cause a considerable slowdown of the visualization for a sensor with a typical number of channels.
* `background_color` : Color of the background of the viewer. Defaults to *white*.
* `view_style` : Style to use to display the elements in the geometry. Options are **wireframe** and **surface**. By default, all elements are displayed as solid surface.
* `opacity` : Default opacity percentage of all detector elements, only used if the *view_style* is set to display solid surfaces. The default value is 0.4, giving a moderate amount of opacity.
* `display_trajectories` : Determines if the trajectories of the primary and secondary particles should be displayed. Defaults to *true*.
* `hidden_trajectories` : Determines if the trajectories should be hidden inside the detectors. Only used if the *display_trajectories* is enabled. Default value of the parameter is true.
* `trajectories_color_mode` : Configures the way, trajectories are colored. Options are either **generic** which colors all trajectories in the same way, **charge** which bases the color on the particle's charge, or **particle** which colors the trajectory based on the type of the particle. The default setting is *charge*.
* `trajectories_color` : Color of the trajectories if *trajectories_color_mode* is set to **generic**. Default value is *blue*.
* `trajectories_color_positive` : Visualization color for positively charged particles. Only used if *trajectories_color_mode* is equal to **charge**. Default is *blue*.
* `trajectories_color_neutral` : Visualization color for neutral particles. Only used if *trajectories_color_mode* is equal to **charge**. Default is *green*.
* `trajectories_color_negative` : Visualization color for negatively charged particles. Only used if *trajectories_color_mode* is equal to **charge**. Default is *red*.
* `trajectories_particle_colors` : Array of combinations of particle ID and color used to determine the particle colors if *trajectories_color_mode* is equal to **particle**. Refer to the Geant4 documentation \[[@g4particles]\] for details about the IDs of particles.
* `trajectories_draw_step` : Determines if the steps of the trajectories should be plotted. Enabled by default. Only used if *display_trajectories* is enabled.
* `trajectories_draw_step_size` : Size of the markers used to display a trajectory step. Defaults to 2 points. Only used if *trajectories_draw_step* is enabled.
* `trajectories_draw_step_color` : Color of the markers used to display a trajectory step. Default value *red*. Only used if *trajectories_draw_step* is enabled.
* `draw_hits` : Determines if hits in the detector should be displayed. Defaults to false. Option is only useful if Geant4 hits are generated in a module.
* `macro_init` : Optional Geant4 macro to execute during initialization. Whenever possible, the configuration parameters above should be used instead of this option.
* `display_limit` : Sets the `displayListLimit` of the visualization GUI, in case the geometry which has to be loaded is too complex for the GUI to be displayed with the current size Display List. Defaults to 1000000.
* `line_segments` : Sets the number of line segments to approximate a circle with. This parameter can be used when simulating radial detectors to visualize their curved edges with more precision. Defaults to 250.
* `viewpoint_thetaphi` : Sets the theta and phi angles for the viewpoint. Defaults to -70deg 20deg.

## Usage

An example configuration providing a wireframe viewing style with the same color for every particle and displaying the result after every event for 2s is provided below:

```ini
[VisualizationGeant4]
mode = "none"
view_style = "wireframe"
trajectories_color_mode = "generic"
accumulate = 0
accumulate_time_step = 2s
```

To view event number N of a simulation, use a fixed random seed and `number_of_events = 1` and `skip_events = N-1` under the `[Allpix]` header. Note that executing `/run/beamOn 1` in the Qt visualization window or Geant4 terminal does **not** show the next event of the Allpix Squared simulation, but rather a random event.

[@g4drivers]: https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/Visualization/visdrivers.html
[@g4particles]: https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/particle.html
