## RCEWriter
**Maintainer**: Salman Maqbool (<salman.maqbool@cern.ch>)   
**Status**: Functional   
**Input**: PixelHit

#### Description
Reads in the Pixel hit messages and saves track data in the RCE format, appropriate for the Proteus telescope reconstruction software. An event tree and a sensor tree and their branches are initialized when the module first runs. The event tree is initialized with the appropriate branches, while a sensor tree is created for each detector and the brances initialized from a strcut. Initially, the program loops over all the pixel hit messages, and then over all the hits in the message, and writes data to the tree branches in the RCE Format. If there are no hits, the event is saved with nHits = 0, with the other fields empty.

#### Parameters
* `file_name` : Name of the data file (without the .root suffix) to create, relative to the output directory of the framework. The default filename is *rce_data.root*

#### Usage
To create the default file (with the name *rce_data.root*) an instantiation without arguments can be placed at the end of the configuration:

```ini
[RCEWriter]
```
