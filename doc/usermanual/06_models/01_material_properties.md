---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Sensor Material Properties"
weight: 1
---

Allpix Squared supports the definition of a variety of semiconductor sensor materials. To simplify the setup of simulations
with certain materials and to avoid inconsistent results, a set of default material properties is defined for each available
material. These stored values serve as defaults to modules depending on one of these properties and may thus be overwritten
using the corresponding configuration key in the respective section of the main configuration file.

The following parameters are currently provided by the framework:

- Charge creation energy

- Fano factor

The values for various materials are listed in the table below. It should be noted that for many of the following values a
significant variation on measurements exist throughout literature, among others owed to a variation of material quality and
composition and of vendors. The sources for the chosen default values are provided in the table.

| Material                                             | Energy $`[eV]`$ | Fano factor | References                             |
|:-----------------------------------------------------|:----------------|:------------|:---------------------------------------|
| Silicon                                              | 3.64            | 0.115       | \[[@chargecreation], [@fano]\]         |
| Germanium                                            | 2.97            | 0.112       | \[[@Germanium_Creation_Fano]\]         |
| Gallium Arsenide                                     | 4.2             | 0.14        | \[[@GaAs_Fano]\]                       |
| Gallium Nitride                                      | 8.33            | 0.07        | \[[@GaN_Creation_Fano]\]               |
| Cadmium Telluride                                    | 4.43            | 0.24        | \[[@CdTe_Creation], [@CdTe_Fano]\]     |
| Cadmium Zinc Telluride $`(\ce{Cd_{0.8}Zn_{0.2}Te})`$ | 4.6             | 0.14        | \[[@CdZnTe_Creation], [@CdZnTe_Fano]\] |
| Diamond                                              | 13.1            | 0.382       | \[[@Diamond_Creation_Fano]\]           |
| Silicon Carbide $`(\ce{{4H-}SiC})`$                  | 7.6             | 0.1         | \[[@SiC_Creation], [@SiC_Fano]\]       |
| Cesium Lead Bromide (CsPbBr<sub>3</sub>)                      | 5.3             | 0.1         | \[[@CsPbBr3_Creation],[@CsPbBr3_Fano]\]|                                        |

It should be noted that material properties such as the density and composition of materials are defined only in case of
constructing a Geant4 geometry via the `GeometryBuilderGeant4` module, therefore these values are implemented within the
respective module.


[@chargecreation]: https://doi.org/10.1103/PhysRevB.1.2945
[@fano]: https://doi.org/10.1103/PhysRevB.22.5565
[@Germanium_Creation_Fano]: https://doi.org/10.1016/0883-2889(91)90002-I
[@GaAs_Fano]: https://doi.org/10.1063/1.1406546
[@GaN_Creation_Fano]: https://etd.ohiolink.edu/apexprod/rws_etd/send_file/send?accession=osu1448405475
[@CdTe_Creation]: https://doi.org/10.1016/0029-554X(74)90662-4
[@CdTe_Fano]: https://doi.org/10.1016/j.nima.2018.09.025
[@CdZnTe_Creation]: https://doi.org/10.1016/j.astropartphys.2021.102563
[@CdZnTe_Fano]: https://doi.org/10.1109/23.322857
[@Diamond_Creation_Fano]: https://doi.org/10.1002/pssa.201600195
[@SiC_Creation]: https://doi.org/10.1109/NSSMIC.2005.1596542
[@SiC_Fano]: https://doi.org/10.1016/j.nima.2010.08.046
[@CsPbBr3_Creation]: https://doi.org/10.1038/s41467-018-04073-3
[@CsPbBr3_Fano]: https://doi.org/10.1038/s41566-020-00727-1
