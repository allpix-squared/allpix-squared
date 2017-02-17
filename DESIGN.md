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
* ~~Key-value pairs from the top part of the file without sections are considered global and passed to all modules~~ (key value pairs should only inherit from classes with a lower priority or defined higher up in the configuration? --- problem is the constructor initialization which makes it difficult to know beforehand if priority is higher and what is the full configuration that needs to be passed)
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

### Message Interface

| Possible instance                                                                                           | Physics example                                                                       | Message implementation                                                                                                                                                                                                         | Clipboard implementation                                                                                                                                                                                   |
|-------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Single instance of a single module without input                                                            | ---                                                                                   | (**easy**) Add to run queue in init                                                                                                                                                                                                | (**easy**) Run directly                                                                                                                                                                                        |
| Single instance of multiple modules with a single input triggering only once                                | Single detector deposition, collection and write to file (assuming always hit)        | (**easy**) First add to run queue, passes message to others                                                                                                                                                                        | (**easy/moderate**) Run in linear way if configuration is correctly ordered                                                                                                                                    |
| Multiple instance of multiple modules with a single input triggering once linearly                          | --- (not relevant as particle may not hit every detector)                             | (**easy**) Add to run queue in init and resolve detectors with multiple instances that receive their own message                                                                                                                   | (**easy/moderate**) Run in linear way if configuration is correctly ordered and fetch every object for a specific detector                                                                                     |
| Multiple instance of multiple modules with a single triggering none or once                                 | Multiple detectors deposition and write every detector separately to file             | (**easy**) Some messages are never triggered, but that is just because there is no data                                                                                                                                            | (**moderate**) Run in linear way if configuration is correctly ordered and fetch + check that object is actually available for a detector                                                                      |
| Single instance of multiple modules with  multiple inputs triggering once                                   | --- (not relevant as particle may not hit every detector)                             | (**easy**) Add to run queue in init, resolve multiple messages and run after a module got all of its messages                                                                                                                      | (**moderate**) Run correctly if the configuration order is valid (it is a valid toposort)                                                                                                                      |
| Multiple instance of multiple modules with multiple inputs once triggering not or once                      | Single detector deposition, propagation and write output for both of them to one file | (**moderate/hard**) either (1) disallow and always send an empty message or (2) require the sending of a ‘module finished’ message or (3) emulate a dry-run to find a valid order the modules should be run (special empty object) | (**moderate**) Run correctly if the configuration order is valid and fetch + check every object to detect if it has been correctly produced or should be ignored                                               |
| Multiple instance of multiple modules with not always but sometimes multiple times triggering single inputs | Multiple detector deposition written to a single file                                 | (**moderate/hard**) see above and  variables cannot be bound to a message as more may arrive before module runs (list of message should be used instead, should be disallowed unless specifically enabled)                         | (**moderate/hard**) see above, storage should allow for storage of vector of messages too and users should be possible to specify if something can be a vector or should never be  (disallowed unless enabled) |
| Multiple same modules with different input, output collection (ignoring above)                              | Run same module at once on a single                                                   | (**easy**) Disallow two outputs to reach the same input (unless specifically allowed) (**hard**) (1) Users should handle vector of messages or (2) the messaging system should dynamically clone an instance                           | (**easy**) Disallow two outputs to reach the same input (unless specifically allowed) (**hard**) Users should handle vector of messages                                                                            |
| Multithreading on events                                                                                    | Run multiple events at once                                                           | (**easy**) If no timing or history is involved (**hard**) If modules should have history                                                                                                                                               | (**easy**) If no timing or history is involved (**hard**) If modules should have history                                                                                                                           |
| Multithreading on modules in single event                                                                   | Run multiple modules at once                                                          | (**easy/moderate**) Works as long as AllPix is threadsafe (module is not important)                                                                                                                                                | (**hard**) The linear scheduler has no idea what to run in parallel                                                                                                                                            |

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
