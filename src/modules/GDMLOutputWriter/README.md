# GDMLOutputWriter
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)  
**Status**: Functional

### Description
Constructs a GDML output file of the geometry if this module is added.
This feature is to be considered experimental as the GDML implementation of Geant4 is incomplete.

### Dependencies

This module requires an installation `Geant4_GDML`. This option can be enabled by configuring and compiling Geant4 with the option `-DGEANT4_USE_GDML=ON`

### Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension .gdml will be appended if not present. Defaults to `Output.gdml`

### Usage
Creating a GDML output file  with the name myOutputfile.gdml

[GDMLOutputWriter]
file_name = myOutputfile
```
