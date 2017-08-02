## LCIOWriter
**Maintainer**: Andreas Nurnberg (andreas.nurnberg@cern.ch)  
**Status**: Functional  
**Input**: *PixelHitMessage*

#### Description
Writes pixel hit data to LCIO file, compatible to EUTelescope analysis framework.

#### Parameters
* `file_name`: LCIO file to write. Extension .slcio
* `pixelType`: EUtelescope pixel type to create. Default: 2 EUTelGenericSparsePixel.
  - EUTelSimpleSparsePixelDefault 1
  - EUTelGenericSparsePixel 2
  - EUTelTimepix3SparsePixel 5
* DetectorName
* OutputCollectionName

#### Usage
```ini
[LCIOWriter]
```
