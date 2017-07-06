## ROOTObjectWriter
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Input**: *all objects in simulation*

#### Description
Reads in all the messages dispatched by the framework that contain AllPix objects (which should normally be all messages). Every of those messages contain a vector of objects, which are converted to a vector to pointers of the object base class. The first time a new type of object is received a new tree is created having the class name of this object as name. Then for every combination of detector and message name a new branch is created in this tree. A leaf is automatically created for every member of the object. The vector of objects is then written to the file for every event it is dispatched (saving an empty vector if that event did not include the specific object). 

If the same type of messages is dispatched multiple times, it is combined and written to the same tree. Thus the information about those separate messages is lost. It is also currently not possible to limit the data that is written to the file. If only a subset of the objects is needed than the rest of the data should be discarded afterwards.

#### Parameters
* `file_name` : Name of the data file (without the .root suffix) to create, relative to the output directory of the framework.

#### Usage
To create the default file (with the name *data.root*) an instantiation without arguments can be placed at the end of the configuration:

```ini
[ROOTObjectWriter]
```
