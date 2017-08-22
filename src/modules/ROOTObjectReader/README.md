## ROOTObjectReader
**Maintainer**: Koen Wolters (<koen.wolters@cern.ch>)  
**Status**: Functional  
**Output**: *all objects in input file*

#### Description
Converts all the object data stored in the ROOT data file produced by ROOTObjectWriter back in to messages (see the description of ROOTObjectWriter for more information about the format). Reads all the trees defined in the data file that contain Allpix objects. Creates a message from the objects in the tree for every event (as long as the file contains the same number of events as used in the simulation). 

#### Parameters
* `file_name` : Location of the ROOT file containing the trees with the object data.
* `include` : Array of object names (without the allpix prefix) to read from the ROOT trees, all other object names are ignored (cannot be used together simulateneously with the *exclude* parameter).
* `exclude`: Array of object names (without the allpix prefix) that are not read from the ROOT trees (cannot be used together simulateneously with the *include* parameter).

#### Usage
This module should be put at the beginning of the main configuration. An example to read only the PixelCharge and the PixelHit objects from the file *data.root* is given below:

```ini
[ROOTObjectReader]
file_name = "data.root"
include = "PixelCharge", "PixelHit"
```
