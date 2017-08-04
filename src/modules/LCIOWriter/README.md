## LCIOWriter
**Maintainer**: Andreas Nurnberg (andreas.nurnberg@cern.ch)  
**Status**: Functional  
**Input**: *PixelHitMessage*

#### Description
Writes pixel hit data to LCIO file, compatible to EUTelescope analysis framework.

#### Parameters
* `file_name`: LCIO file to write. Extension .slcio
* `pixel_type`: EUtelescope pixel type to create. Default: 2 EUTelGenericSparsePixel.
  - EUTelSimpleSparsePixelDefault 1
  - EUTelGenericSparsePixel 2
  - EUTelTimepix3SparsePixel 5
* `Detector_name`: Detector name written to the run header. Default: "EUTelescope"
* `output_collection_name`: Name of the LCIO collection containing the pixel data. Default: "zsdata_m26"

#### Usage
```ini
[LCIOWriter]
```
