/// \file TGeoBuilderModule.cpp
/// \brief Implementation of the TGeoBuilderModule class
/// \author N. Gauvin

/* Usage
   TGeoBuilderModule* c = new TGeoBuilderModule()

   To be discussed :
   - Shall the algo stops if a geometry is already loaded ?
   - Do we want a CheckOverlaps option ? Or we use ROOT's tools offline on the TFile.
   - TGeoBuilderModule also responsible for loading the geometry ?

   Colors :
   kOrange+1 : experimental hall
   kRed      : wrapper
   kCyan     : Wafer, pixels
   kGreen    : PCB, bumps container volumes
   kYellow   : Bump logical volume
   kGray     : Chip
   kBlack    : Appliances
*/

// Local includes
#include "TGeoBuilderModule.hpp"

// AllPix includes
#include "core/utils/log.h"
#include "tools/ROOT.h"
#include "tools/geant4.h"

// Global includes
#include <iostream>
#include <memory>

// ROOT
#include <TColor.h>
#include <TFile.h>
#include <TGeoBBox.h>
#include <TGeoCompositeShape.h>
#include <TGeoSphere.h>
#include <TGeoTube.h>
#include <TROOT.h>

using namespace std;
using namespace allpix;

/// Name of the module
const std::string TGeoBuilderModule::name = "geometry_tgeo";

/// Constructor and destructor
TGeoBuilderModule::TGeoBuilderModule(AllPix* apx, Configuration conf)
    : Module(apx), m_fillingWorldMaterial(nullptr), m_geoMap(), m_userDefinedWorldMaterial("Air"),
      m_userDefinedGeoOutputFile(""), m_buildAppliancesFlag(false), m_Appliances_type(0), m_buildTestStructureFlag(false),
      m_vectorWrapperEnhancement(), m_posVector(), m_rotVector(), m_posVectorAppliances(), m_config(std::move(conf)) {
    // read the configuration
    // WARNING: these conversion go wrong without include tools/ROOT.h - prefer to use std::string
    m_userDefinedWorldMaterial = m_config.get<TString>("world_material");
    m_userDefinedGeoOutputFile = m_config.get<TString>("output_file", "");

    m_buildAppliancesFlag = m_config.get<bool>("build_appliances", false);
    if(m_buildAppliancesFlag) {
        m_Appliances_type = m_config.get<int>("appliances_type");
    }
    m_buildTestStructureFlag = m_config.get<bool>("build_test_structures", false);
}
TGeoBuilderModule::~TGeoBuilderModule() = default;

/// Run the detector construction.
void TGeoBuilderModule::run() {

    // Import and load external geometry
    // TGeoManager::Import("MyGeom.root");
    // Return

    // Set class parameters according to users' specifications.
    // Koen...
    /* For development purpose only */

    // Read detector description.
    // Koen...
    ReadDetectorDescriptions();

    /* Instantiate the TGeo geometry manager.
       It will remain persistant until gGeoManager is deleted.
     */
    gGeoManager = new TGeoManager("AllPix2", "Detector geometry");
    // Set Verbosity according to the framework. 0=Mute, 1=verbose
    gGeoManager->SetVerboseLevel(1);

    // Build detectors.
    Construct();

    // Close the Geometry
    gGeoManager->CloseGeometry();

    //### Visualisation Development only
    TGeoVolume* top = gGeoManager->GetTopVolume();
    // gGeoManager->SetTopVisible(); // the TOP is invisible by default
    gGeoManager->SetVisLevel(3);
    // gGeoManager->SetVisOption(0); // To see the intermediate containers.
    // gGeoManager->GetVolume("name");
    top->Draw();
    // gGeoManager->CheckOverlaps(0.1);

    // Save geometry in ROOT file.
    if(m_userDefinedGeoOutputFile.Length() != 0) {
        if(!m_userDefinedGeoOutputFile.EndsWith(".root")) {
            m_userDefinedGeoOutputFile += ".root";
        }
        gGeoManager->Export(m_userDefinedGeoOutputFile); // ("file.root","","update") ??
        LOG(DEBUG) << "Geometry saved in " << m_userDefinedGeoOutputFile;
    }

    // Export geometry as GDML.
    // gGeoManager->Export("MyGeom.gdml");
}

/*
 * The master function to construct the detector according to the user's wishes.
 */
void TGeoBuilderModule::Construct() {

    // Solids will be builds in mm, same units as AllPix1, even if ROOT assumes cm.
    // Beware when computing shape capacity or volume weight.

    LOG(DEBUG) << "Starting construction of the detector geometry.";

    // Create the materials and media.
    BuildMaterialsAndMedia();

    /* Creating the world volume, ie experimental hall
       The size of the world does not seem to have any effect. Even if smaller than
       the built detectors, ROOT does not complain.
    */
    G4ThreeVector halfworld = m_config.get("half_world", G4ThreeVector(100, 100, 100));

    const double halfworld_dx = halfworld.x(); // mm
    const double halfworld_dy = halfworld.y(); // mm
    const double halfworld_dz = halfworld.z(); // mm

    m_fillingWorldMaterial = gGeoManager->GetMedium(m_userDefinedWorldMaterial);
    // If null, throw an exception and stop the construction !
    if(m_fillingWorldMaterial == nullptr) {
        throw ModuleException("Material " + std::string(m_userDefinedWorldMaterial) +
                              " requested to fill the world volume does not exist");
    } else {
        LOG(DEBUG) << "Using " << m_userDefinedWorldMaterial << " to fill the world volume.";
    }

    // World volume, ie the experimental hall.
    TGeoVolume* expHall_log =
        gGeoManager->MakeBox("ExpHall", m_fillingWorldMaterial, halfworld_dx, halfworld_dy, halfworld_dz);
    // expHall_log->SetTransparency(100);
    // G4Color(1.0, 0.65, 0.0, 0.1)->kOrange+1, SetVisibility(false), SetForceSolid(false)
    expHall_log->SetLineColor(kOrange + 1);
    gGeoManager->SetTopVolume(expHall_log);

    // Build the pixel detectors
    BuildPixelDevices();

    // Build appliances
    if(m_buildAppliancesFlag) {
        BuildAppliances();
    }

    // Build test structures
    if(m_buildTestStructureFlag) {
        BuildTestStructure();
    }

    LOG(DEBUG) << "Construction of the detector geometry successful.";
}

void TGeoBuilderModule::BuildPixelDevices() {

    LOG(DEBUG) << "Starting construction of the pixel detectors.";

    int global_id_cnt = 0;

    // Big loop on pixel detectors.
    LOG(DEBUG) << "Building " << m_geoMap.size() << " device(s) ...";
    auto detItr = m_geoMap.begin();
    for(; detItr != m_geoMap.end(); detItr++) {
        std::shared_ptr<PixelDetectorModel> dsc = *detItr;
        int id = global_id_cnt++;
        TString id_s = Form("_%i", id);
        LOG(DEBUG) << "Start detector " << id;

        ///////////////////////////////////////////////////////////
        // wrapper
        // The wrapper might be enhanced when the user set up
        //  Appliances to the detector (extra layers, etc).
        double wrapperHX = dsc->GetHalfWrapperDX();
        double wrapperHY = dsc->GetHalfWrapperDY();
        double wrapperHZ = dsc->GetHalfWrapperDZ();

        // Apply the enhancement to the medipixes (to contain possible appliances)
        // We can have N medipixes and K enhancements, where K<=N.
        // For instance, for 2 medipixes.  We can have.
        // medipix 1 --> with enhancement
        // medipix 2 --> no enhancement
        TGeoTranslation wrapperEnhancementTransl = TGeoTranslation("WrapperEnhancementTransl", 0., 0., 0.);
        if(m_vectorWrapperEnhancement.find(id) != m_vectorWrapperEnhancement.end()) {
            wrapperHX += m_vectorWrapperEnhancement[id].x() / 2.; // half
            wrapperHY += m_vectorWrapperEnhancement[id].y() / 2.;
            wrapperHZ += m_vectorWrapperEnhancement[id].z() / 2.;
            wrapperEnhancementTransl.SetDx(m_vectorWrapperEnhancement[id].x() / 2.);
            wrapperEnhancementTransl.SetDy(m_vectorWrapperEnhancement[id].y() / 2.);
            wrapperEnhancementTransl.SetDz(m_vectorWrapperEnhancement[id].z() / 2.);
        }
        LOG(DEBUG) << "Wrapper Dimensions [mm] : "
                   << TString::Format("hX=%3.3f hY=%3.3f hZ=%3.3f", wrapperHX, wrapperHY, wrapperHZ);

        // The wrapper logical volume
        TGeoVolume* wrapper_log =
            gGeoManager->MakeBox(WrapperName + id_s, m_fillingWorldMaterial, 2. * wrapperHX, 2. * wrapperHY, 2. * wrapperHZ);
        // G4Color(1,0,0,0.9)->kRed, SetLineWidth(1), SetForceSolid(false), SetVisibility(false)
        wrapper_log->SetLineColor(kRed);

        // Placement ! Starting at user position --> vector pos
        TGeoTranslation posWrapper = m_posVector[id];
        // Apply wrapper enhancement
        posWrapper.Add(&wrapperEnhancementTransl);
        TGeoVolume* expHall_log = gGeoManager->GetTopVolume();
        auto* det_tr = new TGeoCombiTrans(posWrapper, m_rotVector[id]);
        det_tr->SetName("DetPlacement" + id_s);
        expHall_log->AddNode(wrapper_log, 1, det_tr);

        ///////////////////////////////////////////////////////////
        // Device
        // The Si wafer is placed respect to the wrapper.
        // Needs to be pushed -half Si wafer in z direction

        TGeoBBox* Wafer_box =
            new TGeoBBox(WaferName + id_s, dsc->GetHalfSensorX(), dsc->GetHalfSensorY(), dsc->GetHalfSensorZ());

        TGeoMedium* Si_med = gGeoManager->GetMedium("Si"); // Retrieve Silicon
        TGeoVolume* Wafer_log = new TGeoVolume(WaferName + id_s, Wafer_box, Si_med);
        // G4Color(0,1,1,1)->kCyan, SetLineWidth(2), SetForceSolid(true);
        Wafer_log->SetLineColor(kCyan);
        Wafer_log->SetLineWidth(2);
        // Wafer_log->SetVisibility(true);

        ///////////////////////////////////////////////////////////
        // slices and pixels
        // Replication along X axis, creation of a family.
        // Option "N" tells to divide the whole axis range into NPixelsX.
        // Start and step arguments are dummy.
        TGeoVolume* Slice_log = Wafer_log->Divide(SliceName + id_s, 1, dsc->GetNPixelsX(), 0, 1, 0, "N");
        // Slice_log->SetVisibility(false);
        // Replication along Y axis
        TGeoVolume* Pixel_log = Slice_log->Divide(PixelName + id_s, 2, dsc->GetNPixelsY(), 0, 1, 0, "N");
        Pixel_log->SetLineColor(kCyan);
        // Pixel_log->SetVisibility(false);
        /*
      The path to the corresponding nodes will be
      Wafer_id_1\Slice_id_[1,NPixelsX]\Pixel_id_[1,NPixelsY]
        */

        // Placement of the Device (Wafer), containing the pixels
        TGeoTranslation* posDevice = new TGeoTranslation("LocalDevTranslation" + id_s, 0., 0., 0.);
        // Apply position Offset for the detector due to the enhancement
        posDevice->Add(&wrapperEnhancementTransl);
        wrapper_log->AddNode(Wafer_log, 1, posDevice);
        // LOG(DEBUG) << "- Sensor position      : " << posDevice;

        ///////////////////////////////////////////////////////////
        // Bumps
        // Bump = Bump_Sphere + Bump_Tube
        // Naming AllPix Allpix2
        // Bump_Box     -> None
        // m_Bumps_log  -> Bumps_log
        // m_Bumps_phys -> None
        // aBump        -> Bump
        // aBump_Sphere -> Bump_Sphere
        // aBump_Tube   -> Bump_Tube
        // m_Bumps_Cell_log -> Bumps
        double bump_height = dsc->GetBumpHeight();
        if(bump_height != 0. && dsc->GetHalfChipZ() != 0.) {

            // Build the basic shapes
            TString BumpSphereName = BumpName + "Sphere" + id_s;
            new TGeoSphere(BumpSphereName,
                           0,                   // internal radius
                           dsc->GetBumpRadius() // ext radius
                           );
            TString BumpTubeName = BumpName + "Tube" + id_s;
            new TGeoTube(BumpTubeName,
                         0., // internal radius
                         // external radius
                         dsc->GetBumpRadius() - dsc->GetBumpDr(),
                         bump_height / 2.);
            // Bump = Bump_Sphere + Bump_Tube
            TGeoCompositeShape* Bump =
                new TGeoCompositeShape(BumpName + "Shape" + id_s, BumpSphereName + "+" + BumpTubeName);

            // The volume containing the bumps
            TGeoVolume* Bumps_log = gGeoManager->MakeBox(BumpName + "Log" + id_s,
                                                         m_fillingWorldMaterial,
                                                         dsc->GetHalfSensorX(),
                                                         dsc->GetHalfSensorY(),
                                                         bump_height / 2.);
            // G4Color(0,1,0,1.0)=kGreen, SetLineWidth(1), SetForceSolid(false),
            // SetVisibility(true)
            Bumps_log->SetLineColor(kGreen);

            // Placement of the volume containing the bumps
            TGeoTranslation* posBumps =
                new TGeoTranslation("LocalBumpsTranslation" + id_s,
                                    0.,
                                    0.,
                                    -dsc->GetHalfSensorZ() - 2 * dsc->GetHalfCoverlayerZ() - (bump_height / 2));
            posBumps->Add(posDevice);
            wrapper_log->AddNode(Bumps_log, 1, posBumps);

            // A bump logical volume
            TGeoMedium* solder_med = gGeoManager->GetMedium("Solder");
            TGeoVolume* Bumps = new TGeoVolume(BumpName + id_s, Bump, solder_med);
            // G4Color::Yellow(), SetLineWidth(2), SetForceSolid(true)
            Bumps->SetLineColor(kYellow);
            Bumps->SetLineWidth(2);

            // Replication and positionning of the bumps.
            // Loop on x axis
            for(int ix = 0; ix < dsc->GetNPixelsX(); ++ix) {

                // Loop on y axis
                for(int iy = 0; iy < dsc->GetNPixelsY(); ++iy) {

                    // Positions
                    double XPos = (ix * 2 + 1) * dsc->GetHalfPixelX() - dsc->GetHalfSensorX() + dsc->GetBumpOffsetX();
                    double YPos = (iy * 2 + 1) * dsc->GetHalfPixelY() - dsc->GetHalfSensorY() + dsc->GetBumpOffsetY();
                    TString xy_s = Form("_%i_%i", ix, iy);
                    TGeoTranslation* posBump = new TGeoTranslation("LocalBumpTranslation" + id_s + xy_s, XPos, YPos, 0.);

                    // Placement !
                    Bumps_log->AddNode(Bumps, ix + 1 + (iy * dsc->GetNPixelsX()), posBump);

                } // end loop y axis

            } // end loop x axis

        } // end if bumps

        ///////////////////////////////////////////////////////////
        // Chip
        // The Si wafer is placed respect to the wrapper.
        // Needs to be pushed -half Si wafer in z direction
        if(dsc->GetHalfChipZ() != 0) {
            TGeoVolume* Chip_log =
                gGeoManager->MakeBox(ChipName + id_s, Si_med, dsc->GetHalfChipX(), dsc->GetHalfChipY(), dsc->GetHalfChipZ());
            // G4Color::Gray(), SetLineWidth(2), SetForceSolid(true), SetVisibility(true)
            Chip_log->SetLineColor(kGray);
            Chip_log->SetLineWidth(2);

            // Placement !
            TGeoTranslation* posChip =
                new TGeoTranslation("LocalChipTranslation" + id_s,
                                    dsc->GetChipXOffset(),
                                    dsc->GetChipYOffset(),
                                    dsc->GetChipZOffset() - dsc->GetHalfSensorZ() - 2. * dsc->GetHalfCoverlayerZ() -
                                        bump_height - dsc->GetHalfChipZ());
            posChip->Add(posDevice);
            wrapper_log->AddNode(Chip_log, 1, posChip);
            // LOG(DEBUG) << "- Chip position        : " << posChip;
        }

        ///////////////////////////////////////////////////////////
        // PCB
        // The PCB is placed respect to the wrapper.
        // Needs to be pushed -half Si wafer in z direction
        if(dsc->GetHalfPCBZ() != 0) {

            // Retrieve Plexiglass
            TGeoMedium* plexiglass_med = gGeoManager->GetMedium("Plexiglass");
            // Create logical volume
            TGeoVolume* PCB_log = gGeoManager->MakeBox(
                PCBName + id_s, plexiglass_med, dsc->GetHalfPCBX(), dsc->GetHalfPCBY(), dsc->GetHalfPCBZ());
            // G4Color::Green(), SetLineWidth(1), SetForceSolid(true)
            PCB_log->SetLineColor(kGreen);

            // Placement !
            TGeoTranslation* posPCB = new TGeoTranslation("LocalPCBTranslation" + id_s,
                                                          -dsc->GetSensorXOffset(),
                                                          -dsc->GetSensorYOffset(),
                                                          -dsc->GetHalfSensorZ() - 2. * dsc->GetHalfCoverlayerZ() -
                                                              bump_height - 2. * dsc->GetHalfChipZ() - dsc->GetHalfPCBZ());
            posPCB->Add(posDevice);
            wrapper_log->AddNode(PCB_log, 1, posPCB);
            // LOG(DEBUG) << "- PCB position         : " << posPCB;

        } // end if PCB

        ///////////////////////////////////////////////////////////
        // Coverlayer if requested (typically made of Al, but user configurable)
        if(dsc->IsCoverlayerON()) {

            /*
              Find out about the material that the user requested.
              This material has to be defined in the BuildMaterialsAndMedia().
              If not, as in AllPix1, a warning is issued and Aluminium is used.
              ### Change that policy ?
             */
            TGeoMedium* Cover_med = gGeoManager->GetMedium(dsc->GetCoverlayerMat().c_str());
            if(Cover_med == nullptr) {
                LOG(WARNING) << "Requested material for the coverlayer " << dsc->GetCoverlayerMat()
                             << " was not found in the material database. "
                             << "Check the spelling or add it in BuildMaterialsAndMedia()."
                             << "Going on with aluminum.";
                Cover_med = gGeoManager->GetMedium("Al");
            }

            // Create logical volume
            TGeoVolume* Cover_log = gGeoManager->MakeBox(
                CoverName + id_s, Cover_med, dsc->GetHalfSensorX(), dsc->GetHalfSensorY(), dsc->GetHalfCoverlayerZ());
            // G4Color::White() !!, SetLineWidth(2), SetForceSolid(true)
            // ROOT background is withe by default. Change White into ...
            Cover_log->SetLineWidth(2);

            // Placement !
            TGeoTranslation* posCover = new TGeoTranslation(
                "LocalCoverlayerTranslation" + id_s, 0., 0., -dsc->GetHalfSensorZ() - dsc->GetHalfCoverlayerZ());
            posCover->Add(posDevice);
            wrapper_log->AddNode(Cover_log, 1, posCover);

        } // end if Coverlayer

        ///////////////////////////////////////////////////////////
        //  GuardRings and excess area
        // Guard rings will be GuardRingsExt - Box
        TString GuardRingsExtName = GuardRingsName + "Ext" + id_s;
        new TGeoBBox(GuardRingsExtName,
                     dsc->GetHalfSensorX() + dsc->GetSensorExcessHRight() + dsc->GetSensorExcessHLeft(),
                     dsc->GetHalfSensorY() + dsc->GetSensorExcessHTop() + dsc->GetSensorExcessHBottom(),
                     // same depth as the sensor
                     dsc->GetHalfSensorZ());

        TGeoCompositeShape* Solid_GuardRings = new TGeoCompositeShape(GuardRingsName + id_s,
                                                                      // GuardRings = GuardRings_Ext - Wafer
                                                                      GuardRingsExtName + "-" + Wafer_box->GetName());

        // Create logical volume
        TGeoVolume* GuardRings_log = new TGeoVolume(GuardRingsName + id_s, Solid_GuardRings, Si_med);
        // G4Color(0.5,0.5,0.5,1)=kGray+2, SetLineWidth(1), SetForceSolid(true)
        GuardRings_log->SetLineColor(kGray + 2);

        // Placement ! Same as device
        wrapper_log->AddNode(GuardRings_log, 1, posDevice);

    } // Big loop on detector descriptions

    LOG(DEBUG) << "Construction of the pixel detector successful.";
}

void TGeoBuilderModule::BuildAppliances() {

    // Through the comand
    // --> /allpix/extras/setAppliancePosition
    // you can fill the vector "m_posVectorAppliances" available in this scope.
    // This vector holds the positions of the appliances volumes which can be placed with
    // respect to the wrapper.  This way your appliance properly rotates
    // with the detector.

    // Through the comand
    // --> /allpix/extras/setWrapperEnhancement
    // you can enhance the size of the wrapper so daughter volumens of the wrappers
    // fit in.

    LOG(DEBUG) << "Starting construction of the appliances " << m_Appliances_type;

    // Check that appliance type is valid.
    if(m_Appliances_type < 0 || m_Appliances_type > 1) {
        LOG(DEBUG) << "Unknown Appliance Type : " << m_Appliances_type
                   << "Available types are 0,1. Set /allpix/extras/setApplianceType accordingly."
                   << "Quitting...";
        return;
    }

    // Check that we have some position vectors for the appliances.
    if(m_posVectorAppliances.empty()) {
        LOG(DEBUG) << "You requested to build appliances, but no translation vector given."
                   << "Please, set /allpix/extras/setAppliancePosition accordingly."
                   << "Abandonning...";
        return;
    }

    // Retrieve medium, ie aluminium.
    TGeoMedium* Al = gGeoManager->GetMedium("Al");

    // Build shapes and translations according to the requested type.
    TString comp;                          // The composition of shapes.
    TGeoTranslation* ApplTransl = nullptr; // Type-depending Translation vector.
    if(m_Appliances_type == 0) {

        new TGeoBBox("AppBoxSup", 87. / 2, 79. / 2, 5);
        new TGeoBBox("AppBoxSupN", 72. / 2, 54. / 2, 8.);
        new TGeoBBox("AppBoxSupN2", 52. / 2, 54. / 2, 5.);

        auto* BoxSupN2Transl = new TGeoTranslation("AppBoxSupN2Translation", 0., 44.5, 4.);
        BoxSupN2Transl->RegisterYourself();
        comp = "(AppBoxSup-AppBoxSupN)-AppBoxSupN2:AppBoxSupN2Translation";

        // Type depending translation vectors, with respect to the wrapper.
        ApplTransl = new TGeoTranslation("ApplianceTransl", 0., 10.25, 0.);

    } else if(m_Appliances_type == 1) {

        // Empty Aluminium box with a window.

        // Create the composite shape. mm !
        new TGeoBBox("AppBoxOut", 54. / 2, 94.25 / 2, 12. / 2);
        new TGeoBBox("AppBoxIn", 52.5 / 2, 92.5 / 2, 12. / 2);
        new TGeoBBox("AppWindow", 10., 10., 1.5);

        auto BoxInTransl = new TGeoTranslation("AppBoxInTranslation", 0., 0., -1.5);
        BoxInTransl->RegisterYourself();
        auto* WindowTransl = new TGeoTranslation("AppWindowTranslation", 0., -22.25, 6.);
        WindowTransl->RegisterYourself();

        comp = "(AppBoxOut-AppBoxIn:AppBoxInTranslation)-AppWindow:AppWindowTranslation";

        // Type depending translation vectors, with respect to the wrapper.
        ApplTransl = new TGeoTranslation("ApplianceTransl", 0., 0., 11.2);
    }

    auto* Support = new TGeoCompositeShape("SupportBox", comp);
    // Create logical volume
    auto* Support_log = new TGeoVolume("Appliance", Support, Al);
    // G4Color(0,0,0,0.6)=kBlack,SetLineWidth(2),SetForceSolid(true),SetVisibility(true)
    Support_log->SetLineWidth(2);
    Support_log->SetLineColor(kBlack);

    // Loop on the given position vectors and position the volumes.
    auto aplItr = m_posVectorAppliances.begin();
    for(; aplItr != m_posVectorAppliances.end(); aplItr++) {
        int detId = (*aplItr).first;
        TString id_s = "_" + std::to_string(detId);

        // Translation vectors, with respect to the wrapper.
        // equals type-depending translation plus user given translation.
        TGeoTranslation* ApplTranslItr = new TGeoTranslation("ApplianceTransl" + id_s, 0., 0., 0.);
        ApplTranslItr->Add(&m_posVectorAppliances[detId]);
        ApplTranslItr->Add(ApplTransl);

        // Creation of the node.
        // The mother volume is the wrapper. It will rotate with the wrapper.
        TGeoVolume* Wrapper_log = gGeoManager->GetVolume(WrapperName + id_s);
        Wrapper_log->AddNode(Support_log, detId, ApplTranslItr);

    } // end loop positions

    /*

    switch(m_Appliances_type){
    case 0:
      {

        // Create the composite shape. mm !
        TGeoBBox* BoxSup = new TGeoBBox("AppBoxSup",87./2, 79./2, 5);
        TGeoBBox* BoxSupN = new TGeoBBox("AppBoxSupN", 72./2, 54./2, 8.);
        TGeoBBox* BoxSupN2 = new TGeoBBox("AppBoxSupN2", 52./2, 54./2, 5.);

        TGeoTranslation* BoxSupN2Transl =
      new TGeoTranslation("AppBoxSupN2Translation", 0., 44.5, 4. );
        TString comp = "(AppBoxSup-AppBoxSupN)-AppBoxSupN2:AppBoxSupN2Translation";
        TGeoCompositeShape* Support = new TGeoCompositeShape("SupportBox",comp);

        // Create logical volume
        TGeoVolume* Support_log = new TGeoVolume("Appliance",Support,Al);
        //G4Color(0,0,0,0.6)=kBlack,SetLineWidth(2),SetForceSolid(true),SetVisibility(true)
        Support_log->SetLineWidth(2);
        Support_log->SetLineColor(kBlack);

        // Loop on the given position vectors and position the volumes.
        map<int,TGeoTranslation>::iterator aplItr = m_posVectorAppliances.begin();
        for( ; aplItr != m_posVectorAppliances.end() ; aplItr++) {
      int detId = (*aplItr).first;
      TString id_s = Form("_%i",detId);

      // Translation vectors, with respect to the wrapper.
      TGeoTranslation* ApplTransl =
        new TGeoTranslation("ApplianceTransl"+id_s,0.,10.25,0.);
      ApplTransl->Add(&m_posVectorAppliances[detId]);

      // Creation of the node.
      // The mother volume is the wrapper. It will rotate with the wrapper.
      TGeoVolume* Wrapper_log = gGeoManager->GetVolume(WrapperName+id_s);
      Wrapper_log->AddNode(Support_log,detId,ApplTransl);

        } // end loop positions


      }
    case 1:
      {
        // Empty Aluminium box with a window.

        // Create the composite shape. mm !
        TGeoBBox* BoxOut = new TGeoBBox("AppBoxOut", 54./2, 94.25/2, 12./2 );
        TGeoBBox* BoxIn = new TGeoBBox("AppBoxIn", 52.5/2, 92.5/2, 12./2 );
        TGeoBBox* Window = new TGeoBBox("AppWindow", 10., 10., 1.5 );

        TGeoTranslation* BoxInTransl =
      new TGeoTranslation("AppBoxInTranslation", 0., 0., -1.5 );
        TGeoTranslation* WindowTransl =
      new TGeoTranslation("AppWindowTranslation", 0., -22.25, 6. );

        TString comp = "(AppBoxOut-AppBoxIn:BoxInTransl)-AppWindow:AppWindowTranslation";
        TGeoCompositeShape* Support = new TGeoCompositeShape("SupportBox",comp);

        // Create logical volume
        TGeoVolume* Support_log = new TGeoVolume("Appliance",Support,Al);
        //G4Color(255,0,0,0.7)=kRed,SetLineWidth(2),SetForceSolid(true),SetVisibility(true)
        Support_log->SetLineWidth(2);
        Support_log->SetLineColor(kRed);

        // Loop on the given position vectors and position the volumes.
        map<int,TGeoTranslation>::iterator aplItr = m_posVectorAppliances.begin();
        for( ; aplItr != m_posVectorAppliances.end() ; aplItr++) {
      int detId = (*aplItr).first;
      TString id_s = Form("_%i",detId);

      // Translation vectors, with respect to the wrapper.
      TGeoTranslation* ApplTransl =
        new TGeoTranslation("ApplianceTransl"+id_s,0.,0.,11.2);
      ApplTransl->Add(&m_posVectorAppliances[detId]);

      // Creation of the node.
      // The mother volume is the wrapper. It will rotate with the wrapper.
      TGeoVolume* Wrapper_log = gGeoManager->GetVolume(WrapperName+id_s);
      Wrapper_log->AddNode(Support_log,detId,ApplTransl);

        } // end loop positions
      } // end case 1
    default:
      {
        LOG(DEBUG) << "Unknown Appliance Type";
        break;
      }

    }
    */

    LOG(DEBUG) << "Construction of the appliances successful.";
}

void TGeoBuilderModule::BuildTestStructure() {}

/*
  Create the materials and media.
 */
void TGeoBuilderModule::BuildMaterialsAndMedia() {

    /* Create the materials and mediums
       Important note :
       Only simple elements and materials are defined and used, enough for the
       geometry description.
       It is to the user's responsability to map those elements during the
       simulation phase to elements with the proper physical properties.
       Example : "Air" to "G4_Air", which could not be reproduced here.
    */

    int numed = 0; // user medium index

    // Vacuum
    // G4Material("Vacuum", z=1 , a=1.01*g/mole, density= 0.0001*g/cm3);
    int z = 1;
    double a = 1.01;         // g/mole
    double density = 0.0001; // g/cm3
    auto* vacuum_mat = new TGeoMaterial("Vacuum", a, z, density);
    new TGeoMedium("Vacuum", 1, vacuum_mat);

    // Air
    /* AllPix1 uses "G4_AIR"
  Material:   G4_AIR    density:  1.205 mg/cm3  RadL: 303.921 m    Nucl.Int.Length: 710.095 m
                         Imean:  85.700 eV   temperature: 293.15 K  pressure:   1.00 atm

     --->  Element: C (C)   Z =  6.0   N =    12   A = 12.011 g/mole
           --->  Isotope:   C12   Z =  6   N =  12   A =  12.00 g/mole   abundance: 98.930 %
           --->  Isotope:   C13   Z =  6   N =  13   A =  13.00 g/mole   abundance:  1.070 %
            ElmMassFraction:   0.01 %  ElmAbundance   0.02 %

     --->  Element: N (N)   Z =  7.0   N =    14   A = 14.007 g/mole
           --->  Isotope:   N14   Z =  7   N =  14   A =  14.00 g/mole   abundance: 99.632 %
           --->  Isotope:   N15   Z =  7   N =  15   A =  15.00 g/mole   abundance:  0.368 %
            ElmMassFraction:  75.53 %  ElmAbundance  78.44 %

     --->  Element: O (O)   Z =  8.0   N =    16   A = 15.999 g/mole
           --->  Isotope:   O16   Z =  8   N =  16   A =  15.99 g/mole   abundance: 99.757 %
           --->  Isotope:   O17   Z =  8   N =  17   A =  17.00 g/mole   abundance:  0.038 %
           --->  Isotope:   O18   Z =  8   N =  18   A =  18.00 g/mole   abundance:  0.205 %
            ElmMassFraction:  23.18 %  ElmAbundance  21.07 %

     --->  Element: Ar (Ar)   Z = 18.0   N =    40   A = 39.948 g/mole
           --->  Isotope:  Ar36   Z = 18   N =  36   A =  35.97 g/mole   abundance:  0.337 %
           --->  Isotope:  Ar38   Z = 18   N =  38   A =  37.96 g/mole   abundance:  0.063 %
           --->  Isotope:  Ar40   Z = 18   N =  40   A =  39.96 g/mole   abundance: 99.600 %
            ElmMassFraction:   1.28 %  ElmAbundance   0.47 %
    */
    auto* N = new TGeoElement("Nitrogen", "N", z = 7, a = 14.007);
    auto* O = new TGeoElement("Oxygen", "O", z = 8, a = 15.999);
    auto* C = new TGeoElement("Carbon", "C", z = 6, a = 12.011);
    auto* Ar = new TGeoElement("Argon", "Ar", z = 18, a = 39.948);
    auto* air_mat = new TGeoMixture("Air", 4, density = 1.205E-3);
    air_mat->AddElement(N, 0.7844);
    air_mat->AddElement(O, 0.2107);
    air_mat->AddElement(C, 0.0002);
    air_mat->AddElement(Ar, 0.0047);
    new TGeoMedium("Air", ++numed, air_mat);

    /* Silicon
       AllPix1 uses "G4_Si"
    */
    TGeoElementTable* table = gGeoManager->GetElementTable();
    TGeoElement* Si = table->FindElement("Si");
    auto* Si_mat = new TGeoMaterial("Si", Si, density = 2.330);
    new TGeoMedium("Si", ++numed, Si_mat);

    /* Epoxy
       AllPix1 uses G4_PLEXIGLASS
    */
    TGeoElement* H = table->FindElement("H");
    auto* plexiglass_mat = new TGeoMixture("Plexiglass", 3, density = 1.19);
    plexiglass_mat->AddElement(C, 5);
    plexiglass_mat->AddElement(H, 8);
    plexiglass_mat->AddElement(O, 2);
    new TGeoMedium("Plexiglass", ++numed, plexiglass_mat);

    /* Solder SnPb */
    auto* Sn = new TGeoElement("Tin", "Sn", z = 50, a = 118.710);
    auto* Pb = new TGeoElement("Lead", "Pb", z = 82., a = 207.2);
    auto* solder_mat = new TGeoMixture("Solder", 2, density = 8.4);
    solder_mat->AddElement(Sn, 63);
    solder_mat->AddElement(Pb, 37);
    new TGeoMedium("Solder", ++numed, solder_mat);

    /* Aluminum
       AllPix1 uses G4_Al
    */
    TGeoElement* Al = table->FindElement("Al");
    auto* Al_mat = new TGeoMaterial("Al", Al, density = 2.699);
    new TGeoMedium("Al", ++numed, Al_mat);
}

/*
  Dummy function that fills one GeoDsc class. For development purpose only.
 */
void TGeoBuilderModule::ReadDetectorDescriptions() {

    // Create new GeoDsc
    auto dsc = std::make_shared<PixelDetectorModel>("tgeo_test");

    // Fill it with data from macros/OneFEI4_vis.in
    /*
  Dump geo description for object with Id : 200
   Digitizer         : FEI3Standard
     npix_x            = 80
     npix_y            = 336
     npix_z            = 0
     pixsize_x         = 0.125 [mm]
     pixsize_y         = 0.025
     pixsize_z         = 0.125
     sensor_hx         = 10, posx -4.3
     sensor_hy         = 8.4, posy 28.2
     sensor_hz         = 0.125, posz 0
     coverlayer_hz     = 0
     coverlayer_mat    = G4Al
     chip_hx           = 10, posx 0
     chip_hy           = 8.4, posy 0
     chip_hz           = 0.195, posz 0
     pcb_hx            = 26.5
     pcb_hy            = 47
     pcb_hz            = 0.8
    */
    // dsc->SetSensorDigitizer("FEI3Standard");
    // dsc->SetID(200);
    dsc->SetNPixelsX(80);
    dsc->SetNPixelsY(336);
    dsc->SetNPixelsZ(0);
    dsc->SetPixSizeX(0.125);
    dsc->SetPixSizeY(0.025);
    dsc->SetPixSizeZ(0.125);
    dsc->SetSensorHX(10);
    dsc->SetSensorHY(8.4);
    dsc->SetSensorHZ(0.125);
    dsc->SetSensorPosX(-4.3);
    dsc->SetSensorPosY(28.2);
    dsc->SetSensorPosZ(0);
    // SetCoverlayerHZ(0);
    // SetCoverlayerMat("Al");
    dsc->SetChipHX(10);
    dsc->SetChipHY(8.4);
    dsc->SetChipHZ(0.195);
    dsc->SetChipPosX(0);
    dsc->SetChipPosY(0);
    dsc->SetChipPosZ(0);
    dsc->SetPCBHX(26.5);
    dsc->SetPCBHY(47);
    dsc->SetPCBHZ(0.8);

    m_geoMap.push_back(dsc);

    /*
      /allpix/det/setPosition   0.0 0.0  0.0  mm
      /allpix/det/setRotation   0.0 0.0  0.0 deg
      Position/Rotation of the detector with respect to the world coordinates
    */
    m_posVector[200] = TGeoTranslation(0.0, 0.0, 0.0);
    // m_posVector[200] = TGeoTranslation(10., 10., 10.);
    m_posVector[200].SetName("DetTranslation_200"); // For ROOT's records
    m_rotVector[200] = TGeoRotation();              // Identity
    m_rotVector[200].SetName("DetRotation_200");    // For ROOT's records
    // G4 style :
    // m_rotVector[200] = TGeoRotation(-0.0 180.0 180.0);//deg
    // m_vectorWrapperEnhancement[200] = TVector3(5.0, 5.0, 5.0);

    // Test appliances
    m_Appliances_type = 0;
    m_buildAppliancesFlag = false;
    m_posVectorAppliances[200] = TGeoTranslation(0.0, 0.0, 0.0);
}

/******************* OLD STUFF ****************************/

/*
    ///////////////////////////////////////////////////////////////////////
    // vis attributes
    G4VisAttributes * pixelVisAtt= new G4VisAttributes(G4Color::Blue());
    pixelVisAtt->SetLineWidth(1);
    pixelVisAtt->SetForceSolid(true);
    //pixelVisAtt->SetForceWireframe(true);

    G4VisAttributes * BoxVisAtt= new G4VisAttributes(G4Color(0,1,1,1));//kCyan
    BoxVisAtt->SetLineWidth(2);
    BoxVisAtt->SetForceSolid(true);
    //BoxVisAtt->SetVisibility(false);

    G4VisAttributes * CoverlayerVisAtt= new G4VisAttributes(G4Color::White());
    CoverlayerVisAtt->SetLineWidth(2);
    CoverlayerVisAtt->SetForceSolid(true);

    G4VisAttributes * ChipVisAtt= new G4VisAttributes(G4Color::Gray());
    ChipVisAtt->SetLineWidth(2);
    ChipVisAtt->SetForceSolid(true);
    //BoxVisAtt->SetVisibility(false);

    G4VisAttributes * BumpBoxVisAtt = new G4VisAttributes(G4Color(0,1,0,1.0));//kGreen
    BumpBoxVisAtt->SetLineWidth(1);
    BumpBoxVisAtt->SetForceSolid(false);
    BumpBoxVisAtt->SetVisibility(true);

    G4VisAttributes * BumpVisAtt = new G4VisAttributes(G4Color::Yellow());
    BumpVisAtt->SetLineWidth(2);
    BumpVisAtt->SetForceSolid(true);
    //BumpVisAtt->SetVisibility(true);
    //BumpVisAtt->SetForceAuxEdgeVisible(true);

    G4VisAttributes * pcbVisAtt = new G4VisAttributes(G4Color::Green());
    pcbVisAtt->SetLineWidth(1);
    pcbVisAtt->SetForceSolid(true);

    G4VisAttributes * guardRingsVisAtt = new G4VisAttributes(G4Color(0.5,0.5,0.5,1));
    guardRingsVisAtt->SetLineWidth(1);
    guardRingsVisAtt->SetForceSolid(true);

    G4VisAttributes * wrapperVisAtt = new G4VisAttributes(G4Color(1,0,0,0.9));
    wrapperVisAtt->SetLineWidth(1);
    wrapperVisAtt->SetForceSolid(false);
    wrapperVisAtt->SetVisibility(false);

RGBA (red, green, blue, and alpha), alpha=opacity (d=1, opaque!)
2 drawing styles :
wireframe : only the edges of your detector are drawn and so the detector looks transparent.
surfaces : detector looks opaque with shading effects.
Equivalent in ROOT :
Int_t ci = TColor::GetFreeColorIndex();
TColor *color = new TColor(ci, 1.0, 0.65, 0.0,"",0.1 );
myVolume->SetLineColor(kRed);
myVolume->SetLineWith(2);
myVolumeContainer->SetVisibility(kFALSE);
There is the possibility to set the transparency or Alpha for every object. Check TColor.

     Rotation/translation with respect to its mother volume
     m_rotVector[id], posWrapper
     G4VPhysicalVolume* trackerPhys  = new G4PVPlacement(0,  // no rotation
     G4ThreeVector(pos_x, pos_y,pos_z), // translation position
     trackerLog,  // its logical volume
     "Tracker",   // its name
     worldLog,    // its mother (logical) volume
     false,       // no boolean operations
     0);          // its copy number
     /// ROOT ??
     TGeoVolume::AddNode(TGeoVolume *daughter,Int_t copy_No,TGeoMatrix *matr);
     Placement with respect to its mother volume
     TGeoTranslation *tr1 = new TGeoTranslation(20., 0, 0.);
     TGeoRotation   *rot1 = new TGeoRotation("rot1", 90., 0., 90., 270., 0., 0.);
     TGeoCombiTrans *combi1 = new TGeoCombiTrans(transl, rot1);

// Rotation
G4RotationMatrix -> TRotation
// counter-clockwise rotations around the coordinate axes
RotateX(),RotateY() and RotateZ()   // get radians
a.Inverse();// b is inverse of a, a is unchanged
b = a.Invert();// invert a and set b = a

    ///////////////////////////////////////////////////////////////////////
    */
