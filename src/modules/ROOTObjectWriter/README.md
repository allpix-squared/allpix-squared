## ROOTObjectWriter
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Input**: *all objects in simulation*

#### Description
Reads in all the messages dispatched by the framework that contain Allpix objects (which should normally be all messages). Every of those messages contain a vector of objects, which are converted to a vector to pointers of the object base class. The first time a new type of object is received a new tree is created having the class name of this object as name. Then for every combination of detector and message name a new branch is created in this tree. A leaf is automatically created for every member of the object. The vector of objects is then written to the file for every event it is dispatched (saving an empty vector if that event did not include the specific object). If the same type of messages is dispatched multiple times, it is combined and written to the same tree. Information about those separate messages is therefore lost. 

In addition to the objects, both the configuration and the geometry setup are written to the ROOT file. The main configuration file is copied directly and all key/value pairs are written to a directory *config* in a subdirectory with the name of the corresponding module. All the detectors are written to a subdirectory with the name of the detector in the top directory *detectors*. Every detector contains the position, rotation matrix and the detector model (with all key/value pairs stored in a similar way as the main configuration).

#### Parameters
* `file_name` : Name of the data file (without the .root suffix) to create, relative to the output directory of the framework.
* `include` : Array of object names to write to the ROOT trees, all other object names are ignored (cannot be used together simulateneously with the *exclude* parameter).
* `exclude`: Array of object names that are not written to the ROOT trees (cannot be used together simulateneously with the *include* parameter).

#### Usage
To create the default file (with the name *data.root*) an instantiation without arguments can be placed at the end of the configuration:

```ini
[ROOTObjectWriter]
```
