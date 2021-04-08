# eudet RD53a Simulation Example

this example is similar to the eudet telescope example but with extra DUTs added to match the DESY testbeam setup with RD53a modules. The setup consists six telescope planes of MIMOSA26-type (EUDET beam telescope), two rd53a modules centered in between the telescope arms; DUT0 defined with 50x50um^2 pitch and DUT1 defined with 20x100um^2 pitch, and one FEI4 reference plane as the last plane as in real testbeam setup.   

The goal of this setup is to simulate the performance of rd53a modules with testbeam setup and to study multiple scattering effects with passive and extra material. for this purpose the passive material = "Polystyrene" was introduced, the user can also try other materials within the same range of radiation length (such as: styrofoam, plexiglass). The 'GeometryBuilderGeant4' has to be assigned to be able to use the passive material. 

In principal a linear electric field is applied but it could be modified by the user. The `ProjectionPropagation` module is estantiated for the all planes the same as for the EUDET telecope, charge propagation could also be modified by the user for the DUTs.

A 'LCIOWriter' module is added to output an lcio format file that can be used as input for reconstruction using EUTelescope software. For EUTelescope the data must be in 'zsdata' type to be recognized. To reconstruct with corryvreckan the user can istantiate the module `CorryvreckanWriter` similarly.   


