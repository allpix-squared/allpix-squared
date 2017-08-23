## CorryvreckanWriter
**Maintainer**: Daniel Hynds (daniel.hynds@cern.ch)  
**Status**: Functional  
**Input**: PixelHit

#### Description
Takes all digitised pixel hits and converts them into Corryvrekan pixel format. These are then written to an output file in the expected format to be read in by the reconstruction software.

#### Parameters
* `file_name` : Output filename (appended with .root)

#### Usage
Typical usage is:

```ini
[CorryvreckanWriter]
file_name = corryvreckan
```
