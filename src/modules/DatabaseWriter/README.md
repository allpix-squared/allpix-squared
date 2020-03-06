# DatabaseWriter
**Maintainer**: Enrico Junior Schioppa (<enrico.junior.schioppa@cern.ch>)  
**Status**: Functional  
**Input**: *all objects in simulation*

### Description
This module allows one to write the simulation output into a postgreSQL database.

```
=== <event number> ===
```

and individual detectors by the detector marker:

```
--- <detector name> ---
```

The `include` and `exclude` parameters can be used to restrict the objects written to file to a certain type.

### Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension `.txt` will be appended if not present.
* `include` : Array of object names (without `allpix::` prefix) to write to the ASCII text file, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names (without `allpix::` prefix) that are not written to the ASCII text file (cannot be used together simultaneously with the *include* parameter).

### Usage
To create the default file (with the name *data.txt*) containing entries only for PixelHit objects, the following configuration can be placed at the end of the main configuration:

```ini
[DatabaseWriter]
include = "PixelHit"
```
