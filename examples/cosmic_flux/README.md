# Cosmic Muon Flux Simulation
This example illustrates how the *depositionCosmics* module is used to model the flux of cosmic muons in Allpix Squared. Python analysis code is included to calculate the flux through the detector from the *MCParticle* objects

## Usage
First of all, change into the example directory. Because the time output of the CRY simulation code needs to be known for the analysis, we store the log in a *.txt* file:
```bash
allpix -c cosmic_flux.conf >> cosmic_flux_log.txt
```
To analyse the tracks of the *MCParticles*, we convert them into a text list. Open the root framework and issue
```
.L PATH_TO_ALLPIX_INSTALL/lib/libAllpixObjects.so
```
Next, load the conversion macro via
```
.L analysis/rootMacro.cc
```
Finally, execute it with
```
rootMacro()
```
and close root with
```
.q
```
Now you can analyse the result by issuing
```bash
python analysis/analysis.py
```
Notice that the analysis script relies on the python modules *uncertainties*, *parse*, *matplotlib* and *numpy.*
