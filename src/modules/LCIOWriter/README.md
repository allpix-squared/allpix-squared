## LCIOWriter
**Maintainer**: Andreas Nurnberg (<andreas.nurnberg@cern.ch>)   
**Status**: Functional   
**Input**: PixelHit

#### Description
Writes pixel hit data to LCIO file, compatible with the EUTelescope analysis framework [@eutelescope].

#### Parameters
* `file_name`: name of the LCIO file to write, relative to the output directory of the framework. The extension **.slcio** should be added. Defaults to `output.slcio`.
* `pixel_type`: EUtelescope pixel type to create. Options: EUTelSimpleSparsePixelDefault = 1, EUTelGenericSparsePixel = 2, EUTelTimepix3SparsePixel = 5 (Default: EUTelGenericSparsePixel)
* `detector_name`: Detector name written to the run header. Default: "EUTelescope"
* `output_collection_name`: Name of the LCIO collection containing the pixel data. Default: "zsdata_m26"

#### Usage
```ini
[LCIOWriter]
file_name = "run000123-converter.slcio"
```

[@eutelescope]: http://eutelescope.web.cern.ch/
