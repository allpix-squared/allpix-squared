# GDMLOutputWriter
**Maintainer**: Koen van den Brandt (<kbrandt@nikhef.nl>)
**Status**: Functional

### Description
Constructs a GDML output file of the geometry if this module is added.
A new name has to be added for each new output file, the software is unable to overwrite previous files.

### Dependencies

This module requires an installation `Geant4_GDML`. This option can be added by configuring and compiling Geant4 with the option `-DGEANT4_USE_GDML=ON`

### Parameters
* `GDML_ouput_file_name` : sets the name of the name of the GDML output file. Don't include the .gdml extension or the module will give an error. Defaults to `Output`

### Usage
[GDMLOutputWriter]
GDML_output_file_name = myOutput
```
