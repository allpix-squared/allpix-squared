# Design Guidelines for the Development of allpix<sup>2</sup>

This document gives a brief summary of the design guidelines chosen and the priorities in developing features of the framework.

## General Considerations

### Goals

* Reflect the physics
  * Event based - event here refers to particle(s) passage through the setup
  * All of this information must be contained at the very end of processing an event
  * Even "no output" is an information which might be of interest (e.g. no charges deposited)

* Ease of use:
  * Simplicity for module implementation
  * Simple configuration and execution ("does what you expect")

* Modularity
  * Independent modules: write independent code without dependency on anything other but the core
  * Allow very simple but also advanced user configurations

* Flexibility
  * Allow to combine different detectors, different modules
  * Limit code flexibility for the sake of simplicity and ease of use


### Event / Run definition

* A run is everything that happens between the construction and destruction of an AllpixCore object
* An event is everything related to one physical (inter)action with the setup (see above)
* One message will contain all information for one event from the respective module, no partial information will be sent.


## Implementation Details

### Configuration

* Every module receives its own configuration section, indicated by squared backets []
* Configuration object contains alist of key/value pairs
* Configurations will never change during a run (from start to exit of allpix<sup>2</sup>)
* Key-value pairs from the top part of the file without sections are considered global and passed to all modules
* Configuration is structured in sections
* Sections correspond to modules
* The configuration parameters `name` and `type` can be used to assign sections to a certain type of detector or to a specific detector referenced by its name
* Hierarchy: global < Module(all) < Module(type) < Module(name)
* Configuration parameters are inherited from the levels lower in hierarchy

* An other possibility to bear in mind: add the possibility to specify single variables only for one detector name in a lower-hierarchy configuration section:

```
[GenericRKFPropagation]
# Unnamed module section for all detectors
temperature         = 297 # Kelvin
tpx3_2.temperature  = 263 # K - cooled device! 
```

### Module Interface

* Try to abstract most of the functionality for receiving messages and adding to run queue to base class
* Make module interface as simple as possible
* In child class, simply define messages (type e.g.) to listen for
* The remaining work is done by the module base class:
  * add listener
  * provide receive function to add the module to the run queue
* The configuration is directly passed to the constructor of the module class

### End of Event / End of Run

* Cleanup between events (message reception):
  * Only done for things needed din base class
  * Not necessary for user module:
    * Inside `run()` function, things go out of scope and are deleted
    * Module member functions can be used to store things across many events - user has to care about this!


### Libraries

* libAllpixCore
  Contains all core parts and managers of the allpix<sup>2</sup> framework
  
* libAllpixMessages
  Contains all message and data type definitions
  
* Independent libraries for every module