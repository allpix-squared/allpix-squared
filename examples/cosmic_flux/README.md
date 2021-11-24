# Cosmic Muon Flux Simulation
This example illustrates how the *depositionCosmics* module is used to model the flux of cosmic muons in Allpix Squared. Python analysis code is included to calculate the flux through the detector from the *MCParticle* objects

## Usage
First of all, change into the example directory. Because the time output of the CRY simulation code needs to be known for the analysis, we store the log in a *.txt* file:
```bash
allpix -c cosmic_flux.conf >> cosmic_flux_log.txt
```
To analyse the tracks of the *MCParticles*, issue
```bash
python analysis/analysis.py -l PATH_TO_ALLPIX_INSTALL/lib/libAllpixObjects.so
```
The library flag is required when your Allpix installation is not in a standard path.
Notice that the analysis script relies on the python modules *uncertainties*, *tqdm*, *matplotlib* and *numpy.*
