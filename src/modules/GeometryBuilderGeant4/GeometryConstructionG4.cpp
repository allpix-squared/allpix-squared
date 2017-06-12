/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "GeometryConstructionG4.hpp"

#include <memory>
#include <string>
#include <utility>

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
#include <G4NistManager.hh>
#include <G4PVDivision.hh>
#include <G4PVParameterised.hh>
#include <G4PVPlacement.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>
#include <G4UserLimits.hh>
#include <G4VSolid.hh>
#include <G4VisAttributes.hh>
#include "G4StepLimiterPhysics.hh"

#include "core/geometry/PixelDetectorModel.hpp"
#include "core/utils/log.h"
#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "BumpsParameterizationG4.hpp"

// FIXME: should get rid of CLHEP units
#include <CLHEP/Units/SystemOfUnits.h>

using namespace CLHEP;
using namespace std;
using namespace allpix;

// Constructor and destructor
GeometryConstructionG4::GeometryConstructionG4(GeometryManager* geo, const G4ThreeVector& world_size, bool simple_view)
    : geo_manager_(geo), world_size_(world_size), simple_view_(simple_view), world_material_(nullptr), world_log_(nullptr),
      world_phys_(nullptr) {}
GeometryConstructionG4::~GeometryConstructionG4() = default;

// Build geometry
G4VPhysicalVolume* GeometryConstructionG4::Construct() {
    // Initialize materials
    init_materials();

    // Set world material
    world_material_ = materials_["vacuum"];
    LOG(TRACE) << "Material of world " << world_material_->GetName();

    // build the world
    G4Box* world_box = new G4Box("World", world_size_[0], world_size_[1], world_size_[2]);
    world_log_ = new G4LogicalVolume(world_box, world_material_, "World", nullptr, nullptr, nullptr);

    // set the world to invisible in the viewer
    // FIXME: this should strictly not be defined here, but simplifies things a lot
    G4VisAttributes invisibleVisAtt = G4VisAttributes(G4Color(1.0, 0.65, 0.0, 0.1));
    invisibleVisAtt.SetVisibility(false);
    invisibleVisAtt.SetForceSolid(false);
    world_log_->SetVisAttributes(invisibleVisAtt);

    // place the world at the center
    world_phys_ = new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), world_log_, "World", nullptr, false, 0);

    // build the pixel devices
    build_pixel_devices();

    // WARNING: some debug printing here about the placement of the detectors

    return world_phys_;
}

// Initialize all required materials
void GeometryConstructionG4::init_materials() {
    G4NistManager* nistman = G4NistManager::Instance();

    materials_["vacuum"] = new G4Material("Vacuum", 1, 1.01 * g / mole, 0.0001 * g / cm3);
    materials_["air"] = nistman->FindOrBuildMaterial("G4_AIR");

    materials_["silicon"] = nistman->FindOrBuildMaterial("G4_Si");
    materials_["epoxy"] = nistman->FindOrBuildMaterial("G4_PLEXIGLASS");

    // Create solder element
    G4Element* Sn = new G4Element("Tin", "Sn", 50., 118.710 * g / mole);
    G4Element* Pb = new G4Element("Lead", "Pb", 82., 207.2 * g / mole);
    G4Material* Solder = new G4Material("Solder", 8.4 * g / cm3, 2);
    Solder->AddElement(Sn, 63);
    Solder->AddElement(Pb, 37);

    materials_["solder"] = Solder;
}

// WARNING: A DEFINE HERE THAT SHOULD PROBABLY BE A PARAMETER
// NOTE: MULTIPLE ALERT HERE TO IDENTIFY PARTS THAT ARE NOT COPIED FROM ALLPIX 1
void GeometryConstructionG4::build_pixel_devices() {
    /* VISIBILITY
     * set the visibility of several of the volumes
     * FIXME: should strictly not be here but simplifies visualization
     */

    G4VisAttributes BoxVisAtt = G4VisAttributes(G4Color(0, 1, 1, 1));
    BoxVisAtt.SetLineWidth(2);
    BoxVisAtt.SetForceSolid(true);

    G4VisAttributes ChipVisAtt = G4VisAttributes(G4Color::Gray());
    ChipVisAtt.SetLineWidth(2);
    ChipVisAtt.SetForceSolid(true);

    G4VisAttributes BumpBoxVisAtt = G4VisAttributes(G4Color(0, 1, 0, 1.0));
    BumpBoxVisAtt.SetLineWidth(1);
    BumpBoxVisAtt.SetForceSolid(false);
    BumpBoxVisAtt.SetVisibility(true);

    G4VisAttributes BumpVisAtt = G4VisAttributes(G4Color::Yellow());
    BumpVisAtt.SetLineWidth(2);
    BumpVisAtt.SetForceSolid(true);

    G4VisAttributes pcbVisAtt = G4VisAttributes(G4Color::Green());
    pcbVisAtt.SetLineWidth(1);
    pcbVisAtt.SetForceSolid(true);

    G4VisAttributes guardRingsVisAtt = G4VisAttributes(G4Color(0.5, 0.5, 0.5, 1));
    guardRingsVisAtt.SetLineWidth(1);
    guardRingsVisAtt.SetForceSolid(true);

    G4VisAttributes wrapperVisAtt = G4VisAttributes(G4Color(1, 0, 0, 0.9));
    wrapperVisAtt.SetLineWidth(1);
    wrapperVisAtt.SetForceSolid(false);
    wrapperVisAtt.SetVisibility(false);

    /* NAMES
     * define the global names for all the elements in the setup
     */
    pair<G4String, G4String> wrapperName = make_pair("wrapper", "");
    pair<G4String, G4String> PCBName = make_pair("PCB", "");
    pair<G4String, G4String> BoxName = make_pair("Box", "");
    // pair<G4String, G4String> CoverlayerName = make_pair("Coverlayer", "");
    pair<G4String, G4String> SliceName = make_pair("Slice", "");
    pair<G4String, G4String> GuardRingsExtName = make_pair("GuardRingsExt", "");
    pair<G4String, G4String> GuardRingsName = make_pair("GuardRings", "");
    pair<G4String, G4String> PixelName = make_pair("Pixel", "");
    pair<G4String, G4String> ChipName = make_pair("Chip", "");
    pair<G4String, G4String> SDName = make_pair("BoxSD", "");
    pair<G4String, G4String> BumpName = make_pair("Bump", "");
    pair<G4String, G4String> BumpBoxName = make_pair("BumpBox", "");

    /* CONSTRUCTION
     * construct the detectors part by part
     */
    std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
    LOG(TRACE) << "Building " << detectors.size() << " device(s)";

    for(auto& detector : detectors) {
        // get pointers for the model of the detector
        std::shared_ptr<PixelDetectorModel> model = std::dynamic_pointer_cast<PixelDetectorModel>(detector->getModel());

        // ignore all non-pixel detectors
        if(model == nullptr) {
            LOG(ERROR) << "Ignoring detector " << detector->getName()
                       << " because a Geant4 model cannot yet be build for a non-pixel detector";
            continue;
        }

        // NOTE: create temporary internal integer
        // FIXME: this is not necessary unique! if this is necessary generate it internally!
        std::hash<std::string> hash_func;
        int temp_g4_id = static_cast<int>(hash_func(detector->getName()));

        LOG(DEBUG) << "Creating Geant4 model for " << detector->getName();

        /* POSITIONS
         * calculate the positions of all the elements
         */
        G4double bump_height = model->getBumpHeight();

        // positions of all the elements
        G4ThreeVector posCoverlayer(0, 0, 0);
        G4ThreeVector posDevice(0, 0, 0);
        G4ThreeVector posBumps(0, 0, 0);
        G4ThreeVector posChip(0, 0, 0);
        G4ThreeVector posPCB(0, 0, 0);

        // ALERT: NO WRAPPER ENHANCEMENTS
        // apply position offset for the detector due to the enhancement
        /*if ( m_vectorWrapperEnhancement.find(temp_g4_id) != m_vectorWrapperEnhancement.end() ) {
            posDevice.setX(posDevice.x() - m_vectorWrapperEnhancement[*detector].x()/2.);
            posDevice.setY(posDevice.y() - m_vectorWrapperEnhancement[*detector].y()/2.);
            posDevice.setZ(posDevice.z() - m_vectorWrapperEnhancement[*detector].z()/2.);
        } else {*/
        posDevice.setX(posDevice.x());
        posDevice.setY(posDevice.y());
        posDevice.setZ(posDevice.z());
        //}

        // calculation of position of the different physical volumes
        if(model->getHalfChipSizeZ() != 0) {
            posBumps.setX(posDevice.x());
            posBumps.setY(posDevice.y());
            posBumps.setZ(posDevice.z()
                          // wrapperHZ
                          -
                          model->getHalfSensorZ()
                          //- 2.*model->GetHalfCoverlayerZ()
                          -
                          (bump_height / 2.));

            posChip.setX(posDevice.x() + model->getChipOffsetX());
            posChip.setY(posDevice.y() + model->getChipOffsetY());
            posChip.setZ(posDevice.z()
                         // wrapperHZ
                         -
                         model->getHalfSensorZ()
                         //- 2.*model->GetHalfCoverlayerZ()
                         -
                         bump_height - model->getHalfChipSizeZ() + model->getChipOffsetZ());

        } else {
            // make sure no offset because of bumps for PCB is calculated if chip is not included
            bump_height = 0;
        }
        posPCB.setX(posDevice.x() - model->getSensorOffsetX());
        posPCB.setY(posDevice.y() - model->getSensorOffsetY());
        posPCB.setZ(posDevice.z()
                    // wrapperHZ
                    -
                    model->getHalfSensorZ()
                    //- 2.*model->GetHalfCoverlayerZ()
                    -
                    bump_height - 2. * model->getHalfChipSizeZ() - model->getHalfPCBSizeZ());

        LOG(DEBUG) << " Local relative positions of the geometry parts";
        LOG(DEBUG) << " - Coverlayer position  : " << display_vector(posCoverlayer, {"mm", "um"});
        LOG(DEBUG) << " - Sensor position      : " << display_vector(posDevice, {"mm", "um"});
        LOG(DEBUG) << " - Bumps position       : " << display_vector(posBumps, {"mm", "um"});
        LOG(DEBUG) << " - Chip position        : " << display_vector(posChip, {"mm", "um"});
        LOG(DEBUG) << " - PCB position         : " << display_vector(posPCB, {"mm", "um"});

        /* NAMES
         * define the local names of the specific detectors
         */

        std::string name = detector->getName();
        wrapperName.second = wrapperName.first + "_" + name;
        PCBName.second = PCBName.first + "_" + name;
        BoxName.second = BoxName.first + "_" + name;
        // CoverlayerName.second = CoverlayerName.first + "_" + name;
        GuardRingsExtName.second = GuardRingsExtName.first + "_" + name;
        GuardRingsName.second = GuardRingsName.first + "_" + name;
        SliceName.second = BoxName.first + "_" + name;
        PixelName.second = BoxName.first + "_" + name;
        ChipName.second = ChipName.first + "_" + name;
        SDName.second = SDName.first + "_" + name;
        BumpName.second = BumpName.first + "_" + name;
        BumpBoxName.second = BumpBoxName.first + "_" + name;

        /* WRAPPER
         * the wrapper is the box around all of the detector
         *
         * FIXME: this should be centered at the correct place
         */

        // The wrapper might be enhanced when the user set up
        //  Appliances to the detector (extra layers, etc).
        G4double wrapperHX = model->getHalfWrapperDX();
        G4double wrapperHY = model->getHalfWrapperDY();
        G4double wrapperHZ = model->getHalfWrapperDZ();

        LOG(DEBUG) << " Wrapper dimensions : " << Units::display(wrapperHX, "mm") << " " << Units::display(wrapperHY, "mm")
                   << " " << Units::display(wrapperHZ, "mm");

        // Create the wrapper box and logical volume
        auto wrapper_box = std::make_shared<G4Box>(wrapperName.second, 2. * wrapperHX, 2. * wrapperHY, 2. * wrapperHZ);
        auto wrapper_log =
            std::make_shared<G4LogicalVolume>(wrapper_box.get(), world_material_, wrapperName.second + "_log");
        wrapper_log->SetVisAttributes(wrapperVisAtt); // NOTE NOTE
        detector->setExternalObject("wrapper_log", wrapper_log);
        solids_.push_back(wrapper_box);

        // Get position and orientation
        G4ThreeVector posWrapper = toG4Vector(detector->getPosition());
        ROOT::Math::EulerAngles angles = detector->getOrientation();
        auto* rotWrapper = new G4RotationMatrix(angles.Phi(), angles.Theta(), angles.Psi()); // NOTE NOTE

        // ALERT: NO WRAPPER ENHANCEMENTS

        // Place the wrapper
        auto wrapper_phys = std::make_shared<G4PVPlacement>(
            rotWrapper, posWrapper, wrapper_log.get(), wrapperName.second + "_phys", world_log_, false, temp_g4_id, true);
        detector->setExternalObject("wrapper_phys", wrapper_phys);

        /* SENSOR
         * the sensitive detector is the part that collects the deposits
         */

        // Create the sensor box and logical volume
        auto sensor_box = std::make_shared<G4Box>(
            BoxName.second, model->getHalfSensorSizeX(), model->getHalfSensorSizeY(), model->getHalfSensorZ());
        solids_.push_back(sensor_box);
        auto sensor_log =
            std::make_shared<G4LogicalVolume>(sensor_box.get(), materials_["silicon"], BoxName.second + "_log");
        sensor_log->SetVisAttributes(BoxVisAtt); // NOTE NOTE
        detector->setExternalObject("sensor_log", sensor_log);

        auto sensor_phys = std::make_shared<G4PVPlacement>(
            nullptr, posDevice, sensor_log.get(), BoxName.second + "_phys", wrapper_log.get(), false, temp_g4_id, true);
        detector->setExternalObject("sensor_phys", sensor_phys);

        // Create the slice boxes and logical volumes
        auto slice_box = std::make_shared<G4Box>(
            SliceName.first, model->getHalfPixelSizeX(), model->getHalfSensorSizeY(), model->getHalfSensorZ());
        solids_.push_back(slice_box);

        auto slice_log =
            std::make_shared<G4LogicalVolume>(slice_box.get(), materials_["silicon"], SliceName.second); // 0,0,0);
        if(simple_view_) {
            slice_log->SetVisAttributes(G4VisAttributes::GetInvisible()); // NOTE NOTE
        }
        detector->setExternalObject("slice_log", slice_log);

        // Place the slices
        auto slice_div = std::make_shared<G4PVDivision>(
            SliceName.second, slice_log.get(), sensor_log.get(), kXAxis, model->getNPixelsX(), 0);
        detector->setExternalObject("slice_div", slice_div);

        // Create the pixels and logical volumes
        auto pixel_box = std::make_shared<G4Box>(
            PixelName.first, model->getHalfPixelSizeX(), model->getHalfPixelSizeY(), model->getHalfSensorZ());
        solids_.push_back(pixel_box);
        auto pixel_log = std::make_shared<G4LogicalVolume>(pixel_box.get(), materials_["silicon"], PixelName.second);
        detector->setExternalObject("pixel_log", pixel_log);

        if(simple_view_) {
            pixel_log->SetVisAttributes(G4VisAttributes::GetInvisible()); // NOTE NOTE
        }

        // Place the pixels
        auto pixel_div = std::make_shared<G4PVDivision>(
            PixelName.second, pixel_log.get(), slice_log.get(), kYAxis, model->getNPixelsY(), 0);
        detector->setExternalObject("pixel_div", pixel_div);

        /* BUMPS
         * the bump bonds connect the sensor to the readout chip
         */

        // Construct the bumps only if necessary
        if(model->getBumpHeight() > 1e-9 && model->getHalfChipSizeZ() != 0) {
            // Define types from parameters
            G4double bump_sphere_radius = model->getBumpSphereRadius();
            G4double bump_cylinder_radius = model->getBumpCylinderRadius();
            auto bump_sphere =
                std::make_shared<G4Sphere>(BumpName.first + "sphere", 0, bump_sphere_radius, 0, 360 * deg, 0, 360 * deg);
            solids_.push_back(bump_sphere);
            auto bump_tube =
                std::make_shared<G4Tubs>(BumpName.first + "Tube", 0., bump_cylinder_radius, bump_height / 2., 0., 360 * deg);
            solids_.push_back(bump_tube);
            auto bump = std::make_shared<G4UnionSolid>(BumpName.first, bump_sphere.get(), bump_tube.get());
            solids_.push_back(bump);

            // Create the volume containing the bumps
            auto bump_box = std::make_shared<G4Box>(
                BumpBoxName.first, model->getHalfSensorSizeX(), model->getHalfSensorSizeY(), bump_height / 2.);
            solids_.push_back(bump_box);

            // Create the logical wrapper volume
            auto bumps_wrapper_log =
                std::make_shared<G4LogicalVolume>(bump_box.get(), materials_["air"], BumpBoxName.second + "_log");
            bumps_wrapper_log->SetVisAttributes(BumpBoxVisAtt); // NOTE NOTE
            detector->setExternalObject("bumps_wrapper_log", bumps_wrapper_log);

            // Place the general bumps volume
            auto bumps_wrapper_phys = std::make_shared<G4PVPlacement>(nullptr,
                                                                      posBumps,
                                                                      bumps_wrapper_log.get(),
                                                                      BumpBoxName.second + "_phys",
                                                                      wrapper_log.get(),
                                                                      false,
                                                                      temp_g4_id,
                                                                      true);
            detector->setExternalObject("bumps_wrapper_phys", bumps_wrapper_phys);

            // Create the logical volume for the individual bumps
            auto bumps_cell_log =
                std::make_shared<G4LogicalVolume>(bump.get(), materials_["solder"], BumpBoxName.second + "_log");
            if(simple_view_) {
                bumps_cell_log->SetVisAttributes(G4VisAttributes::GetInvisible()); // NOTE NOTE
            } else {
                bumps_cell_log->SetVisAttributes(BumpVisAtt); // NOTE NOTE
            }
            detector->setExternalObject("bumps_cell_log", bumps_cell_log);

            // Create and instantiate a parameterization of the individual bumps
            auto bumps_param = std::make_shared<BumpsParameterizationG4>(model);
            detector->setExternalObject("bumps_param", bumps_param);
            G4int NPixTot = model->getNPixelsX() * model->getNPixelsY();
            auto bumps_param_inst = std::make_shared<G4PVParameterised>(BumpName.second + "phys",
                                                                        bumps_cell_log.get(),
                                                                        bumps_wrapper_log.get(),
                                                                        kUndefined,
                                                                        NPixTot,
                                                                        bumps_param.get());
            detector->setExternalObject("bumps_param_inst", bumps_param_inst);
        }

        // ALERT: NO COVER LAYER

        /* GUARD RINGS
         * rings around the sensitive device
         */

        // Create the box volumes for the guard rings
        auto guard_rings_box = std::make_shared<G4Box>(
            GuardRingsExtName.second,
            model->getHalfSensorSizeX() + (model->getGuardRingExcessRight() + model->getGuardRingExcessLeft()),
            model->getHalfSensorSizeY() + (model->getGuardRingExcessTop() + model->getGuardRingExcessBottom()),
            model->getHalfSensorZ());
        solids_.push_back(guard_rings_box);
        auto guard_rings_solid =
            std::make_shared<G4SubtractionSolid>(GuardRingsName.second, guard_rings_box.get(), sensor_box.get());
        solids_.push_back(guard_rings_solid);

        // Create the logical volume for the guard rings
        auto guard_rings_log = std::make_shared<G4LogicalVolume>(
            guard_rings_solid.get(), materials_["silicon"], GuardRingsName.second + "_log");
        guard_rings_log->SetVisAttributes(guardRingsVisAtt); // NOTE NOTE
        detector->setExternalObject("guard_rings_log", guard_rings_log);

        // Place the guard rings
        auto guard_rings_phys = std::make_shared<G4PVPlacement>(
            nullptr, posDevice, guard_rings_log.get(), GuardRingsName.second + "_phys", wrapper_log.get(), false, 0, true);
        detector->setExternalObject("guard_rings_phys", guard_rings_phys);

        /* CHIPS
         * the chip connected to the bumps bond and the PCB
         */

        // Construct the chips only if necessary
        if(model->getHalfChipSizeZ() != 0) {
            // Create the chip box
            auto chip_box = std::make_shared<G4Box>(
                ChipName.second, model->getHalfChipSizeX(), model->getHalfChipSizeY(), model->getHalfChipSizeZ());
            solids_.push_back(chip_box);

            // Create the logical volume for the chip
            auto chip_log =
                std::make_shared<G4LogicalVolume>(chip_box.get(), materials_["silicon"], ChipName.second + "_log");
            chip_log->SetVisAttributes(ChipVisAtt);
            detector->setExternalObject("chip_log", chip_log);

            // Place the chip
            auto chip_phys = std::make_shared<G4PVPlacement>(
                nullptr, posChip, chip_log.get(), ChipName.second + "_phys", wrapper_log.get(), false, temp_g4_id, true);
            detector->setExternalObject("chip_phys", chip_phys);
        }

        /* PCB
         * global pcb chip
         */

        if(model->getHalfPCBSizeZ() != 0) {
            // Create the box containing the PCB
            auto PCB_box = std::make_shared<G4Box>(
                PCBName.second, model->getHalfPCBSizeX(), model->getHalfPCBSizeY(), model->getHalfPCBSizeZ());
            solids_.push_back(PCB_box);

            // Create the logical volume for the PCB
            auto PCB_log = std::make_shared<G4LogicalVolume>(PCB_box.get(), materials_["epoxy"], PCBName.second + "_log");
            PCB_log->SetVisAttributes(pcbVisAtt);
            detector->setExternalObject("pcb_log", PCB_log);

            // Place the PCB
            auto PCB_phys = std::make_shared<G4PVPlacement>(
                nullptr, posPCB, PCB_log.get(), PCBName.second + "_phys", wrapper_log.get(), false, temp_g4_id, true);
            detector->setExternalObject("pcb_phys", PCB_phys);
        }

        LOG(TRACE) << " Constructed detector " << detector->getName() << " succesfully";
    }
}
