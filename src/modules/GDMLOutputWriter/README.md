# GDMLOutputWriter
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)
**Status**: Functional

### Description
Constructs a GDML output file of the geometry if this module is added.
<<<<<<< HEAD
A new name has to be added for each new output file, the software is unable to overwrite previous files.

### Dependencies

This module requires an installation Geant4_GDML.

### Parameters
* `GDML_ouput_file_name` : sets the name of the name of the GDML output file. Don't include the .gdml extension or the module will give an error. Defaults to `Output`

### Usage
[GDMLOutputWriter]
GDML_output_file_name = myOutput
=======
This feature is to be considered experimental as the GDML implementation of Geant4 is incomplete.

### Dependencies

This module requires an installation `Geant4_GDML`. This option can be enabled by configuring and compiling Geant4 with the option `-DGEANT4_USE_GDML=ON`

### Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension .gdml will be appended if not present. Defaults to `Output.gdml`

### Usage
Creating a GDML output file  with the name myOutputfile.gdml

[GDMLOutputWriter]
file_name = myOutputfile
>>>>>>> 9ad369f6d0a83593a2cab4271b84af331110dfa4
```
