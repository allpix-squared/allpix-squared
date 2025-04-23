/**
 * @file
 * @brief Implementation of material manager for Geant4
 *
 * @copyright Copyright (c) 2021-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "MaterialManager.hpp"

#include "core/module/exceptions.h"
#include "core/utils/text.h"

using namespace allpix;

Materials& Materials::getInstance() {
    static Materials instance;
    return instance;
}

G4Material* Materials::get(const std::string& material) const {

    LOG(TRACE) << "Searching for material \"" << material << "\"";
    // Look in our materials definitions
    // FIXME tolower for this comparison but not for the rest
    auto material_lower = allpix::transform(material, ::tolower);
    if(materials_.find(material_lower) != materials_.end()) {
        LOG(TRACE) << "Found material \"" << material << "\" in internal database";
        return materials_.at(material_lower);
    }

    // If not found, try if we can get it from NIST manager:
    G4NistManager* nistman = G4NistManager::Instance();
    G4Material* nist_material = nullptr;

    // Make sure the material has the "G4_" prefix:
    std::string prefix = "G4_";
    auto res = std::mismatch(prefix.begin(), prefix.end(), material.begin());
    if(res.first == prefix.end()) {
        nist_material = nistman->FindOrBuildMaterial(material);
    } else {
        nist_material = nistman->FindOrBuildMaterial(prefix + material);
    }

    if(nist_material != nullptr) {
        LOG(TRACE) << "Found material \"" << material << "\" in Geant4 NIST Database";
        return nist_material;
    }

    throw ModuleError("Could not find material with name \"" + material + "\"");
}

void Materials::set(const std::string& name, G4Material* material) { materials_.insert(std::make_pair(name, material)); }

/**
 * Initializes all the internal materials. The following materials are supported by this module:
 * - Materials listed by Geant4:
 *   - air
 *   - aluminum
 *   - beryllium
 *   - copper
 *   - kapton
 *   - lead
 *   - lithium
 *   - plexiglass
 *   - silicon
 *   - germanium
 *   - tungsten
 *   - gallium_arsenide
 *   - gallium_nitride
 *   - nickel
 *   - gold
 *   - cadmium_telluride
 *   - titanium
 * - Composite or custom materials:
 *   - carbon fiber
 *   - epoxy
 *   - fused silica
 *   - PCB G-10
 *   - solder
 *   - paper
 *   - polystyrene
 *   - ppo foam
 *   - cadmium zinc telluride
 *   - diamond
 *   - silicon carbide
 *   - titanium grade 5
 *   - vacuum
 *   - cesium_lead_bromide
 */
void Materials::init_materials() {
    G4NistManager* nistman = G4NistManager::Instance();

    // Build table of materials from database
    materials_["air"] = nistman->FindOrBuildMaterial("G4_AIR");
    materials_["aluminum"] = nistman->FindOrBuildMaterial("G4_Al");
    materials_["beryllium"] = nistman->FindOrBuildMaterial("G4_Be");
    materials_["copper"] = nistman->FindOrBuildMaterial("G4_Cu");
    materials_["kapton"] = nistman->FindOrBuildMaterial("G4_KAPTON");
    materials_["lead"] = nistman->FindOrBuildMaterial("G4_Pb");
    materials_["lithium"] = nistman->FindOrBuildMaterial("G4_Li");
    materials_["plexiglass"] = nistman->FindOrBuildMaterial("G4_PLEXIGLASS");
    materials_["silicon"] = nistman->FindOrBuildMaterial("G4_Si");
    materials_["germanium"] = nistman->FindOrBuildMaterial("G4_Ge");
    materials_["tungsten"] = nistman->FindOrBuildMaterial("G4_W");
    materials_["gallium_arsenide"] = nistman->FindOrBuildMaterial("G4_GALLIUM_ARSENIDE");
    materials_["cadmium_telluride"] = nistman->FindOrBuildMaterial("G4_CADMIUM_TELLURIDE");
    materials_["nickel"] = nistman->FindOrBuildMaterial("G4_Ni");
    materials_["gold"] = nistman->FindOrBuildMaterial("G4_Au");
    materials_["titanium"] = nistman->FindOrBuildMaterial("G4_Ti");

    // Get required elements from database
    auto* H = nistman->FindOrBuildElement("H");
    auto* C = nistman->FindOrBuildElement("C");
    auto* N = nistman->FindOrBuildElement("N");
    auto* O = nistman->FindOrBuildElement("O");
    auto* Al = nistman->FindOrBuildElement("Al");
    auto* Si = nistman->FindOrBuildElement("Si");
    auto* Cl = nistman->FindOrBuildElement("Cl");
    auto* Ti = nistman->FindOrBuildElement("Ti");
    auto* V = nistman->FindOrBuildElement("V");
    auto* Zn = nistman->FindOrBuildElement("Zn");
    auto* Ga = nistman->FindOrBuildElement("Ga");
    auto* Cd = nistman->FindOrBuildElement("Cd");
    auto* Sn = nistman->FindOrBuildElement("Sn");
    auto* Te = nistman->FindOrBuildElement("Te");
    auto* Pb = nistman->FindOrBuildElement("Pb");
    auto* Cs = nistman->FindOrBuildElement("Cs");
    auto* Br = nistman->FindOrBuildElement("Br");

    // Create Epoxy material
    auto* Epoxy = new G4Material("Epoxy", 1.3 * CLHEP::g / CLHEP::cm3, 3);
    Epoxy->AddElement(H, 44);
    Epoxy->AddElement(C, 15);
    Epoxy->AddElement(O, 7);
    materials_["epoxy"] = Epoxy;

    // Create Carbon Fiber material:
    auto* CarbonFiber = new G4Material("CarbonFiber", 1.5 * CLHEP::g / CLHEP::cm3, 2);
    CarbonFiber->AddMaterial(Epoxy, 0.4);
    CarbonFiber->AddElement(C, 0.6);
    materials_["carbonfiber"] = CarbonFiber;

    auto* FusedSilica = new G4Material("FusedSilica", 2.2 * CLHEP::g / CLHEP::cm3, 2);
    FusedSilica->AddElement(O, 0.53);
    FusedSilica->AddElement(Si, 0.47);
    materials_["fusedsilica"] = FusedSilica;

    // Create PCB G-10 material
    auto* GTen = new G4Material("G10", 1.7 * CLHEP::g / CLHEP::cm3, 3);
    GTen->AddMaterial(nistman->FindOrBuildMaterial("G4_SILICON_DIOXIDE"), 0.773);
    GTen->AddMaterial(Epoxy, 0.147);
    GTen->AddElement(Cl, 0.08);
    materials_["g10"] = GTen;

    // Create solder material
    auto* Solder = new G4Material("Solder", 8.4 * CLHEP::g / CLHEP::cm3, 2);
    Solder->AddElement(Sn, 0.63);
    Solder->AddElement(Pb, 0.37);
    materials_["solder"] = Solder;

    // Create paper material (cellulose C6H10O5)
    auto* Paper = new G4Material("Paper", 0.8 * CLHEP::g / CLHEP::cm3, 3);
    Paper->AddElement(C, 6);
    Paper->AddElement(O, 5);
    Paper->AddElement(H, 10);
    materials_["paper"] = Paper;

    // Create polystyrene [(C6H5CHCH2)n]
    // https://pdg.lbl.gov/2017/AtomicNuclearProperties/HTML/polystyrene.html
    auto* Polystyrene = new G4Material("Polystyrene", 1.06 * CLHEP::g / CLHEP::cm3, 2);
    Polystyrene->AddElement(C, 8);
    Polystyrene->AddElement(H, 8);
    materials_["polystyrene"] = Polystyrene;

    // Create PPO foam [(C8H8O)n]
    // https://en.wikipedia.org/wiki/Poly(p-phenylene_oxide)
    // (approximate) material for Dortmund Cold Box (DOBOX) used in
    // ATLAS ITk Pixels testbeams
    auto* PPOFoam = new G4Material("PPOFoam", 0.05 * CLHEP::g / CLHEP::cm3, 3);
    PPOFoam->AddElement(C, 8);
    PPOFoam->AddElement(H, 8);
    PPOFoam->AddElement(O, 1);
    materials_["ppofoam"] = PPOFoam;

    // Create Cadmium Zinc Telluride
    auto* CdZnTe = new G4Material("CdZnTe", 5.95 * CLHEP::g / CLHEP::cm3, 3);
    CdZnTe->AddElement(Cd, 8);
    CdZnTe->AddElement(Zn, 2);
    CdZnTe->AddElement(Te, 10);
    materials_["cadmium_zinc_telluride"] = CdZnTe;

    auto* Diamond = new G4Material("Diamond", 6, 12.01 * CLHEP::g / CLHEP::mole, 3.52 * CLHEP::g / CLHEP::cm3);
    materials_["diamond"] = Diamond;

    auto* SiliconCarbide = new G4Material("SiliconCarbide", 3.21 * CLHEP::g / CLHEP::cm3, 2);
    SiliconCarbide->AddElement(Si, 1);
    SiliconCarbide->AddElement(C, 1);
    materials_["silicon_carbide"] = SiliconCarbide;

    auto* GalliumNitride = new G4Material(
        "GalliumNitride", 6.15 * CLHEP::g / CLHEP::cm3, 2); // taken from https://en.wikipedia.org/wiki/Gallium_nitride
    GalliumNitride->AddElement(Ga, 1);
    GalliumNitride->AddElement(N, 1);
    materials_["gallium_nitride"] = GalliumNitride;

    auto* TitaniumGrade5 = new G4Material("Ti5", 4.43 * CLHEP::g / CLHEP::cm3, 3);
    TitaniumGrade5->AddElement(Ti, 0.89875);
    TitaniumGrade5->AddElement(Al, 0.06125);
    TitaniumGrade5->AddElement(V, 0.04);
    materials_["ti5"] = TitaniumGrade5;

    auto* isoB10 = new G4Isotope("B10", 5, 10, 10.012937 * CLHEP::g / CLHEP::mole);
    auto* elB10 = new G4Element("B10", "B", 1);
    elB10->AddIsotope(isoB10, 1);
    auto* B10 = new G4Material("B10", 2.34 * CLHEP::g / CLHEP::cm3, 1);
    B10->AddElement(elB10, 1);
    materials_["b10"] = B10;

    // Add vacuum
    materials_["vacuum"] = new G4Material("Vacuum", 1, 1.008 * CLHEP::g / CLHEP::mole, CLHEP::universe_mean_density);

    // Create Cesium-Lead Bromide
    // Increasingly studied material for hard X-ray detection
    // https://doi.org/10.1063/5.0151902
    // https://doi.org/10.1038/s41598-024-74384-7

    auto* CsPbBr3 = new G4Material("CsPbBr3", 4.42 * CLHEP::g / CLHEP::cm3, 3);
    CsPbBr3->AddElement(Cs, 1);
    CsPbBr3->AddElement(Pb, 1);
    CsPbBr3->AddElement(Br, 3);
    materials_["cesium_lead_bromide"] = CsPbBr3;

}
