## VisualizationGeant4
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  

#### Description
Constructs a visualization viewer to display the constructed Geant4 geometry. The module supports all type of viewers included in Geant4, but the default Qt visualization with the OpenGL viewer is recommended as long as the Geant4 version supports it. 

The module allows for changing a variety of parameters to control the output visualization both for the different detector components and the display of the particle beam.

#### Parameters
* `mode` : Determines the mode of visualization. Options are **gui** which starts a Qt visualization window that contains the driver (as long as the chosen driver supports that), **terminal** starting both the visualization viewer together with a Geant4 terminal or **none** which only starts the driver itself (and directly closes it if the driver is asynchronous). Defaults to **gui**.
* `driver` : Geant4 driver used to visualize the geometry. All the supported options can be found [online](https://geant4.web.cern.ch/geant4/UserDocumentation/UsersGuides/ForApplicationDeveloper/html/ch08s03.html) and depend on the build options of the Geant4 version used. The default **OGL** should normally be used with the **gui** option if the visualization should be accumulated, otherwise **terminal** is the better option. Moreover only the **VRML2FILE** driver has been tested. This driver should be used with *mode* equal to **none**. Defaults to the OpenGL driver **OGL**.
* `accumulate` : Determines if all the events should be accumulated and displayed at the end, or if only the last event should be kept and directly visualized (if the driver supports that). Defaults to true, thus accumulating the events and only displaying the final result.
* `accumulate_time_step` : Time step to sleep every event to allow time to display it if the events are not accumulated. Only used if *accumulate* has been set to disabled. Default value is 100ms.
* `simple_view` : Determines if the visualization should be simplified, not displaying the pixel grid and other parts that are replicated multiple times. Default value is true. This parameter should normally not be changed as it will cause a very huge slowdown of the visualization for a sensor with a typical amount of pixels.
* `background_color` : Color of the background of the viewer. Defaults to white.
* `view_style` : Style to use to display the elements in the geometry. Options include **wireframe** and **surface**. By default the elements are displayed as solid surface.
* `transparency` : Default transparency percentage of all the detector elements, only used if the *view_style* is set to display solids. The default value is 0.4, giving a moderate amount of transparency.
* `display_trajectories` : Determines if the trajectories of the particles should be displayed. Defaults to enabled.
* `hidden_trajectories` : Determines if the trajectory paths should be hidden inside the detectors. Only used if the *display_trajectories* is enabled. Default value of the parameter is true.
* `trajectories_color_mode` : Configures the way the trajectories are colored. First option is **generic**, which colors all trajectories in the same way. The next option is **charge** which bases the color on the charge. The last possible choice **particle**, colors the trajectory based on the type of the particle.
* `trajectories_color` : Color of the trajectories if *trajectories_color_mode* is set to **generic**. Default value is blue.
* `trajectories_color_positive` : Visualization color for positive particles. Only used if *trajectories_color_mode* is equal to **charge**. Default is blue.
* `trajectories_color_neutral` : Visualization color for neutral particles. Only used if *trajectories_color_mode* is equal to **charge**. Default is green.
* `trajectories_color_negative` : Visualization color for negative particles. Only used if *trajectories_color_mode* is equal to **charge**. Default is red.
* `trajectories_particle_colors` : Array of combinations of particle id and color, used to determine the particle colors if if *trajectories_color_mode* is equal to **particle**. Refer to the Geant4 [documentation](http://geant4.cern.ch/G4UsersDocuments/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/particle.html) for details about the ID's of the particles.
* `trajectories_draw_step` : Determines if the steps of the trajectories should be plotted. Defaults to enabled. Only used if the *display_trajectories* is enabled.
* `trajectories_draw_step_size` : Size of the markers used to display a trajectory step. Defaults to 2 points. Only used if the *trajectories_draw_step* is enabled.
* `trajectories_draw_step_color` : Color of the markers used to display a trajectory step. Default value is the color red. Only used if the *trajectories_draw_step* is enabled.
* `draw_hits` : Determines if the hits in the detector should be displayed. Defaults to false. Option is only useful if Geant4 hits are generated in a specific module.
* `macro_init` : Optional Geant4 macro to execute during initialization. Whenever possible the configuration parameters above should be used instead of this option.

#### Usage
An example with a wireframe viewing style with the same color for every particle and displaying the result after every event with 2s waiting time, is the following:

```ini
[VisualizationGeant4]
mode = "none"
view_style = "wireframe"
trajectories_color_mode = "generic"
accumulate = 0
accumulate_time_step = 2s
```
