## LCIOWriter
**Maintainer**: Andreas Nurnberg (andreas.nurnberg@cern.ch)  
**Status**: Immature  
**Input**: *PixelHitMessage*
[**Output**: *OUTPUT_MESSAGE*]  

#### Description
Writes pixel hit data to LCIO file, compatible to EUTelescope analysis framework. Pixel class EUTelGenericSparsePixel hardcoded for now.

#### Parameters
* `file_name`: LCIO file to write. Extension .slcio
* `pixelType`: EUtelescope pixel type to create. Default is 2: EUTelGenericSparsePixel. Does not affect output file (yet).

#### Usage
```ini
[LCIOWriter]
```
