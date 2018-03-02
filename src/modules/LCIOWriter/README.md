# LCIOWriter
**Maintainer**: Andreas Nurnberg (<andreas.nurnberg@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  
**Input**: PixelHit  

### Description
Writes pixel hit data to LCIO file, compatible with the EUTelescope analysis framework [@eutelescope].

If the `geometry_file` parameter is set to a non-empty string, a matching GEAR XML file is created from the simulated detector geometry and written to the simulation output directory. This GEAR file can be used with EUTelescope directly to reconstruct particle trajectories.

### Parameters
* `file_name`: name of the LCIO file to write, relative to the output directory of the framework. The extension **.slcio** should be added. Defaults to `output.slcio`.
* `geometry_file` : name of the output GEAR file to write the EUTelescope geometry description to. Defaults to `allpix_squared_gear.xml`
* `pixel_type`: EUtelescope pixel type to create. Options: EUTelSimpleSparsePixelDefault = 1, EUTelGenericSparsePixel = 2, EUTelTimepix3SparsePixel = 5 (Default: EUTelGenericSparsePixel)
* `detector_name`: Detector name written to the run header. Default: "EUTelescope"
* `output_collection_name`: Name of the LCIO collection containing the pixel data. Default: "zsdata_m26"

### Usage
```ini
[LCIOWriter]
file_name = "run000123-converter.slcio"
```

[@eutelescope]: http://eutelescope.web.cern.ch/
