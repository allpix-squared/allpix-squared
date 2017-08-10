## ROOTObjectReader
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Output**: *all objects in input file*

#### Description
Converts all the object data stored in the ROOT data file produced by ROOTObjectWriter back in to messages (see the description of ROOTObjectWriter for more information about the format). Reads all the trees defined in the data file that contain Allpix objects. Creates a message from the objects in the tree for every event (as long as the file contains the same number of events as used in the simulation). 

Currently it is not yet possible to exclude objects from being read. In case not all objects should be converted to messages, these objects need to be removed from the file before the simulation is started.

#### Parameters
* `file_name` : Location of the ROOT file containing the trees with the object data

#### Usage
This module should be at the beginning of the main configuration. An example to read the objects from the file *data.root* is:

```ini
[ROOTObjectReader]
file_name = "data.root"
```
