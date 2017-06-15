/// \file GeometryBuilderTGeoModule.cpp
/// \brief Implementation of the GeometryBuilderTGeoModule class
/// \author N. Gauvin

/*
  To be discussed :
  - Shall the algo stops if a geometry is already loaded ?
  - Do we want a CheckOverlaps option ? Or we use ROOT's tools offline on the TFile.
    Stop in case of overlap ?
  - GeometryBuilderTGeoModule also responsible for loading the geometry ?

  Colors :
  kOrange+1 : experimental hall
  kRed      : wrapper
  kCyan     : Wafer, pixels
  kGreen    : PCB, bumps container volumes
  kYellow   : Bump logical volume
  kGray     : Chip, GuardRings (+2)
  kBlack    : Appliances
*/

// Local includes
#include "GeometryBuilderTGeoModule.hpp"

// Global includes
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

// ROOT
#include <Math/EulerAngles.h>
#include <Math/Vector3D.h>
#include <TColor.h>
#include <TFile.h>
#include <TGeoBBox.h>
#include <TGeoCompositeShape.h>
#include <TGeoSphere.h>
#include <TGeoTube.h>
#include <TMath.h>
#include <TROOT.h>

// AllPix includes
#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/log.h"

// Tools
#include "tools/ROOT.h"

using namespace std;
using namespace allpix;
using namespace ROOT::Math;
using namespace TMath;

/* Create a TGeoTranslation from a ROOT::Math::XYZVector */
TGeoTranslation ToTGeoTranslation(const XYZPoint& pos) {
    return TGeoTranslation(pos.x(), pos.y(), pos.z());
}
/* Print out a TGeoTranslation as done in allpix for easy comparison. */
TString Print(TGeoTranslation* trl) {
    const Double_t* par = trl->GetTranslation();
    return Form("(%f,%f,%f)", par[0], par[1], par[2]);
}

/// Constructor and destructor
GeometryBuilderTGeoModule::GeometryBuilderTGeoModule(Configuration config, Messenger*, GeometryManager* geo_manager)
    : Module(config), m_config(std::move(config)), m_geoDscMng(geo_manager), m_fillingWorldMaterial(nullptr),
      m_userDefinedWorldMaterial("Air"), m_userDefinedGeoOutputFile(""), m_buildAppliancesFlag(false), m_Appliances_type(0),
      m_buildTestStructureFlag(false) {
    // read the configuration
    // FIXME: prefer to use std::string
    m_userDefinedWorldMaterial = m_config.get<std::string>("world_material");
    m_buildAppliancesFlag = m_config.get<bool>("build_appliances", false);
    if(m_buildAppliancesFlag) {
        m_Appliances_type = m_config.get<int>("appliances_type");
    }
    m_buildTestStructureFlag = m_config.get<bool>("build_test_structures", false);
}

/// Run the detector construction.
void GeometryBuilderTGeoModule::init() {
    // Import and load external geometry
    // TGeoManager::Import("MyGeom.root");
    // Return

    /* Instantiate the TGeo geometry manager.
       It will remain persistant until gGeoManager is deleted.
    */
    gGeoManager = new TGeoManager("AllPix2", "Detector geometry");
    /* Set Verbosity according to the framework. Verbose only in debug mode.
       ROOT : 0=mute, 1=verbose
       LogLevel { QUIET = 0, CRITICAL, ERROR, WARNING, INFO, DEBUG }; */
    gGeoManager->SetVerboseLevel(int(Log::getReportingLevel()) < 5 ? 0 : 1);

    // Build detectors.
    Construct();

    // Close the Geometry
    gGeoManager->CloseGeometry();

    //### Visualisation Development only
    // gGeoManager->SetTopVisible(); // the TOP is invisible by default
    gGeoManager->SetVisLevel(4);
    // gGeoManager->SetVisOption(0); // To see the intermediate containers.
    // gGeoManager->GetVolume("name");
    // TGeoVolume* top = gGeoManager->GetTopVolume();
    // top->Draw();
    // gGeoManager->CheckOverlaps(0.1);

    // Save geometry in ROOT file.
    if(m_config.has("output_file")) {
        m_userDefinedGeoOutputFile = getOutputPath(m_config.get<string>("output_file"));
        if(!m_userDefinedGeoOutputFile.EndsWith(".root")) {
            m_userDefinedGeoOutputFile += ".root";
        }
        gGeoManager->Export(m_userDefinedGeoOutputFile); // ("file.root","","update") ??
        LOG(DEBUG) << "Geometry saved in " << m_userDefinedGeoOutputFile;
    }

    // Export geometry as GDML.
    if(m_config.has("GDML_output_file")) {
#ifndef ROOT_GDML
        std::string error = "You requested to export the geometry in GDML. ";
        error += "However, GDML support is currently disabled in ROOT. ";
        error += "To enable it, configure and compile ROOT with the option -Dgdml=ON.";
        throw allpix::InvalidValueError(m_config, "GDML_output_file", error);
#else
        TString GDML_output_file = getOutputPath(m_config.get<string>("GDML_output_file"));
        if(!GDML_output_file.EndsWith(".gdml")) {
            GDML_output_file += ".gdml";
        }
        gGeoManager->Export(GDML_output_file);
#endif
    }

    //
}

/*
 * The master function to construct the detector according to the user's wishes.
 */
void GeometryBuilderTGeoModule::Construct() {

    // Solids will be builds in mm, same units as AllPix1, even if ROOT assumes cm.
    // Beware when computing shape capacity or volume weight.

    LOG(TRACE) << "Starting construction of the detector geometry.";

    // Create the materials and media.
    BuildMaterialsAndMedia();

    /* Creating the world volume, ie experimental hall
       The size of the world does not seem to have any effect. Even if smaller than
       the built detectors, ROOT does not complain.
    */
    const XYZVector halfworld = m_config.get("half_world", XYZVector(1000, 1000, 1000));

    m_fillingWorldMaterial = gGeoManager->GetMedium(m_userDefinedWorldMaterial);
    // If null, throw an exception and stop the construction !
    if(m_fillingWorldMaterial == nullptr) {
        throw ModuleError("Material " + std::string(m_userDefinedWorldMaterial) +
                          " requested to fill the world volume does not exist");
    } else {
        LOG(TRACE) << "Using " << m_userDefinedWorldMaterial << " to fill the world volume.";
    }

    // World volume, ie the experimental hall.
    TGeoVolume* expHall_log =
        gGeoManager->MakeBox("ExpHall", m_fillingWorldMaterial, halfworld.x(), halfworld.y(), halfworld.z());
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

    LOG(TRACE) << "Construction of the detector geometry successful.";
}

void GeometryBuilderTGeoModule::BuildPixelDevices() {

    LOG(TRACE) << "Starting construction of the pixel detectors.";

    vector<shared_ptr<Detector>> detectors = m_geoDscMng->getDetectors();
    LOG(TRACE) << "Building " << detectors.size() << " device(s) ...";

    // Big loop on pixel detectors.
    auto detItr = detectors.begin();
    for(; detItr != detectors.end(); detItr++) {

        shared_ptr<HybridPixelDetectorModel> dsc = dynamic_pointer_cast<HybridPixelDetectorModel>((*detItr)->getModel());
        string detname = (*detItr)->getName();
        // TString id_s = Form("_%i", id);
        TString id_s = "_";
        id_s += detname;
        LOG(DEBUG) << "Start building detector " << detname;

        ///////////////////////////////////////////////////////////
        // wrapper
        // The wrapper might be enhanced when the user set up
        //  Appliances to the detector (extra layers, etc).
        double wrapperHX = dsc->getHalfWrapperDX();
        double wrapperHY = dsc->getHalfWrapperDY();
        double wrapperHZ = dsc->getHalfWrapperDZ();

        // Apply the enhancement to the medipixes (to contain possible appliances)
        // We can have N medipixes and K enhancements, where K<=N.
        // For instance, for 2 medipixes.  We can have.
        // medipix 1 --> with enhancement
        // medipix 2 --> no enhancement
        TGeoTranslation wrapperEnhancementTransl = TGeoTranslation("WrapperEnhancementTransl", 0., 0., 0.);
        if(m_vectorWrapperEnhancement.find(detname) != m_vectorWrapperEnhancement.end()) {
            wrapperHX += m_vectorWrapperEnhancement[detname].x() / 2.; // half
            wrapperHY += m_vectorWrapperEnhancement[detname].y() / 2.;
            wrapperHZ += m_vectorWrapperEnhancement[detname].z() / 2.;
            wrapperEnhancementTransl.SetDx(m_vectorWrapperEnhancement[detname].x() / 2.);
            wrapperEnhancementTransl.SetDy(m_vectorWrapperEnhancement[detname].y() / 2.);
            wrapperEnhancementTransl.SetDz(m_vectorWrapperEnhancement[detname].z() / 2.);
        }

        // The wrapper logical volume
        TGeoVolume* wrapper_log =
            gGeoManager->MakeBox(WrapperName + id_s, m_fillingWorldMaterial, 2. * wrapperHX, 2. * wrapperHY, 2. * wrapperHZ);
        // G4Color(1,0,0,0.9)->kRed, SetLineWidth(1), SetForceSolid(false), SetVisibility(false)
        wrapper_log->SetLineColor(kRed);

        // Placement ! Retrieve position given by the user.
        TGeoTranslation posWrapper = ToTGeoTranslation((*detItr)->getPosition());
        // Apply wrapper enhancement
        posWrapper.Add(&wrapperEnhancementTransl);
        // Retrieve orientation given by the user.
        EulerAngles angles = (*detItr)->getOrientation();
        const double phi = angles.Phi() * RadToDeg();
        const double theta = angles.Theta() * RadToDeg();
        const double psi = angles.Psi() * RadToDeg();
        TGeoRotation orWrapper = TGeoRotation("DetPlacement" + id_s, phi, theta, psi);
        // And create a transformation.
        auto* det_tr = new TGeoCombiTrans(posWrapper, orWrapper);
        det_tr->SetName("DetPlacement" + id_s);

        // Print out ! The wrapper will just be called "detector".
        LOG(DEBUG) << " Detector placement relative to the World : ";
        LOG(DEBUG) << " - Position             : " << Print(&posWrapper);
        LOG(DEBUG) << " - Orientation          : " << TString::Format("%3.1f %3.1f %3.1f", phi, theta, psi);
        LOG(DEBUG) << " - Wrapper Dimensions   : " << TString::Format("%3.3f %3.3f %3.3f", wrapperHX, wrapperHY, wrapperHZ);

        TGeoVolume* expHall_log = gGeoManager->GetTopVolume();
        expHall_log->AddNode(wrapper_log, 1, det_tr);

        ///////////////////////////////////////////////////////////
        // Device
        // The Si wafer is placed respect to the wrapper.
        // Needs to be pushed -half Si wafer in z direction

        TGeoBBox* Wafer_box = new TGeoBBox(WaferName + id_s,
                                           dsc->getSensorSize().x() / 2.0,
                                           dsc->getSensorSize().y() / 2.0,
                                           dsc->getSensorSize().z() / 2.0);

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
        TGeoVolume* Slice_log = Wafer_log->Divide(SliceName + id_s, 1, dsc->getNPixels().x(), 0, 1, 0, "N");
        // Slice_log->SetVisibility(false);
        // Replication along Y axis
        TGeoVolume* Pixel_log = Slice_log->Divide(PixelName + id_s, 2, dsc->getNPixels().y(), 0, 1, 0, "N");
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
        LOG(DEBUG) << " Relative positions of the elements to the detector :";
        LOG(DEBUG) << " - Sensor position      : " << Print(posDevice);

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
        double bump_height = dsc->getBumpHeight();
        if(bump_height != 0. && dsc->getChipSize().z() / 2.0 != 0.) {

            // Build the basic shapes
            TString BumpSphereName = BumpName + "Sphere" + id_s;
            new TGeoSphere(BumpSphereName,
                           0,                         // internal radius
                           dsc->getBumpSphereRadius() // ext radius
                           );
            TString BumpTubeName = BumpName + "Tube" + id_s;
            new TGeoTube(BumpTubeName,
                         0., // internal radius
                         // external radius
                         dsc->getBumpSphereRadius() - dsc->getBumpCylinderRadius(),
                         bump_height / 2.);
            // Bump = Bump_Sphere + Bump_Tube
            TGeoCompositeShape* Bump =
                new TGeoCompositeShape(BumpName + "Shape" + id_s, BumpSphereName + "+" + BumpTubeName);

            // The volume containing the bumps
            TGeoVolume* Bumps_log = gGeoManager->MakeBox(BumpName + "Log" + id_s,
                                                         m_fillingWorldMaterial,
                                                         dsc->getSensorSize().x() / 2.0,
                                                         dsc->getSensorSize().y() / 2.0,
                                                         bump_height / 2.);
            // G4Color(0,1,0,1.0)=kGreen, SetLineWidth(1), SetForceSolid(false),
            // SetVisibility(true)
            Bumps_log->SetLineColor(kGreen);

            // Placement of the volume containing the bumps
            TGeoTranslation* posBumps = new TGeoTranslation("LocalBumpsTranslation" + id_s,
                                                            0.,
                                                            0.,
                                                            -dsc->getSensorSize().z() / 2.0 -
                                                                2 * dsc->getCoverlayerHeight() / 2.0 - (bump_height / 2));
            posBumps->Add(posDevice);
            LOG(DEBUG) << " - Bumps position       : " << Print(posBumps);
            wrapper_log->AddNode(Bumps_log, 1, posBumps);

            // A bump logical volume
            TGeoMedium* solder_med = gGeoManager->GetMedium("Solder");
            TGeoVolume* Bumps = new TGeoVolume(BumpName + id_s, Bump, solder_med);
            // G4Color::Yellow(), SetLineWidth(2), SetForceSolid(true)
            Bumps->SetLineColor(kYellow);
            Bumps->SetLineWidth(2);

            // Replication and positionning of the bumps.
            // Loop on x axis
            for(int ix = 0; ix < dsc->getNPixels().x(); ++ix) {

                // Loop on y axis
                for(int iy = 0; iy < dsc->getNPixels().y(); ++iy) {

                    // Positions
                    double XPos = (ix * 2 + 1) * dsc->getPixelSize().x() / 2.0 - dsc->getSensorSize().x() / 2.0 +
                                  dsc->getBumpOffset().x();
                    double YPos = (iy * 2 + 1) * dsc->getPixelSize().y() / 2.0 - dsc->getSensorSize().y() / 2.0 +
                                  dsc->getBumpOffset().y();
                    TString xy_s = Form("_%i_%i", ix, iy);
                    TGeoTranslation* posBump = new TGeoTranslation("LocalBumpTranslation" + id_s + xy_s, XPos, YPos, 0.);

                    // Placement !
                    Bumps_log->AddNode(Bumps, ix + 1 + (iy * dsc->getNPixels().x()), posBump);

                } // end loop y axis

            } // end loop x axis

        } // end if bumps

        ///////////////////////////////////////////////////////////
        // Chip
        // The Si wafer is placed respect to the wrapper.
        // Needs to be pushed -half Si wafer in z direction
        if(dsc->getChipSize().z() / 2.0 != 0) {
            TGeoVolume* Chip_log = gGeoManager->MakeBox(ChipName + id_s,
                                                        Si_med,
                                                        dsc->getChipSize().x() / 2.0,
                                                        dsc->getChipSize().y() / 2.0,
                                                        dsc->getChipSize().z() / 2.0);
            // G4Color::Gray(), SetLineWidth(2), SetForceSolid(true), SetVisibility(true)
            Chip_log->SetLineColor(kGray);
            Chip_log->SetLineWidth(2);

            // Placement !
            TGeoTranslation* posChip =
                new TGeoTranslation("LocalChipTranslation" + id_s,
                                    dsc->getChipOffset().x(),
                                    dsc->getChipOffset().y(),
                                    dsc->getChipOffset().z() - dsc->getSensorSize().z() / 2.0 -
                                        2. * dsc->getCoverlayerHeight() / 2.0 - bump_height - dsc->getChipSize().z() / 2.0);
            posChip->Add(posDevice);
            LOG(DEBUG) << " - Chip position        : " << Print(posChip);
            wrapper_log->AddNode(Chip_log, 1, posChip);
        }

        ///////////////////////////////////////////////////////////
        // PCB
        // The PCB is placed respect to the wrapper.
        // Needs to be pushed -half Si wafer in z direction
        if(dsc->getPCBSize().z() / 2.0 != 0) {

            // Retrieve Plexiglass
            TGeoMedium* plexiglass_med = gGeoManager->GetMedium("Plexiglass");
            // Create logical volume
            TGeoVolume* PCB_log = gGeoManager->MakeBox(PCBName + id_s,
                                                       plexiglass_med,
                                                       dsc->getPCBSize().x() / 2.0,
                                                       dsc->getPCBSize().y() / 2.0,
                                                       dsc->getPCBSize().z() / 2.0);
            // G4Color::Green(), SetLineWidth(1), SetForceSolid(true)
            PCB_log->SetLineColor(kGreen);

            // Placement !
            TGeoTranslation* posPCB =
                new TGeoTranslation("LocalPCBTranslation" + id_s,
                                    -dsc->getSensorOffset().x(),
                                    -dsc->getSensorOffset().y(),
                                    -dsc->getSensorSize().z() / 2.0 - 2. * dsc->getCoverlayerHeight() / 2.0 - bump_height -
                                        2. * dsc->getChipSize().z() / 2.0 - dsc->getPCBSize().z() / 2.0);
            posPCB->Add(posDevice);
            LOG(DEBUG) << " - PCB position         : " << Print(posPCB);
            wrapper_log->AddNode(PCB_log, 1, posPCB);

        } // end if PCB

        ///////////////////////////////////////////////////////////
        // Coverlayer if requested (typically made of Al, but user configurable)
        if(dsc->hasCoverlayer()) {

            /*
              Find out about the material that the user requested.
              This material has to be defined in the BuildMaterialsAndMedia().
              If not, as in AllPix1, a warning is issued and Aluminium is used.
              ### Change that policy ?
             */
            TGeoMedium* Cover_med = gGeoManager->GetMedium(dsc->getCoverlayerMaterial().c_str());
            if(Cover_med == nullptr) {
                LOG(ERROR) << "Requested material for the coverlayer " << dsc->getCoverlayerMaterial()
                           << " was not found in the material database. "
                           << "Check the spelling or add it in BuildMaterialsAndMedia()."
                           << "Going on with aluminum.";
                Cover_med = gGeoManager->GetMedium("Al");
            }

            // Create logical volume
            TGeoVolume* Cover_log = gGeoManager->MakeBox(CoverName + id_s,
                                                         Cover_med,
                                                         dsc->getSensorSize().x() / 2.0,
                                                         dsc->getSensorSize().y() / 2.0,
                                                         dsc->getCoverlayerHeight() / 2.0);
            // G4Color::White() !!, SetLineWidth(2), SetForceSolid(true)
            // ROOT background is withe by default. Change White into ...
            Cover_log->SetLineWidth(2);

            // Placement !
            TGeoTranslation* posCover =
                new TGeoTranslation("LocalCoverlayerTranslation" + id_s,
                                    0.,
                                    0.,
                                    -dsc->getSensorSize().z() / 2.0 - dsc->getCoverlayerHeight() / 2.0);
            posCover->Add(posDevice);
            LOG(DEBUG) << " - Coverlayer position  : " << Print(posCover);
            wrapper_log->AddNode(Cover_log, 1, posCover);

        } // end if Coverlayer

        ///////////////////////////////////////////////////////////
        //  GuardRings and excess area
        // Guard rings will be GuardRingsExt - Box
        TString GuardRingsExtName = GuardRingsName + "Ext" + id_s;
        new TGeoBBox(GuardRingsExtName,
                     dsc->getSensorSize().x() / 2.0 + dsc->getGuardRingExcessRight() + dsc->getGuardRingExcessLeft(),
                     dsc->getSensorSize().y() / 2.0 + dsc->getGuardRingExcessTop() + dsc->getGuardRingExcessBottom(),
                     // same depth as the sensor
                     dsc->getSensorSize().z() / 2.0);

        TGeoCompositeShape* Solid_GuardRings = new TGeoCompositeShape(GuardRingsName + id_s,
                                                                      // GuardRings = GuardRings_Ext - Wafer
                                                                      GuardRingsExtName + "-" + Wafer_box->GetName());

        // Create logical volume
        TGeoVolume* GuardRings_log = new TGeoVolume(GuardRingsName + id_s, Solid_GuardRings, Si_med);
        // G4Color(0.5,0.5,0.5,1)=kGray+2, SetLineWidth(1), SetForceSolid(true)
        GuardRings_log->SetLineColor(kGray + 2);

        // Placement ! Same as device
        wrapper_log->AddNode(GuardRings_log, 1, posDevice);

        LOG(TRACE) << "Building detector " << detname << " ... done.";

    } // Big loop on detector descriptions

    LOG(TRACE) << "Construction of the pixel detectors successful.";
}

void GeometryBuilderTGeoModule::BuildAppliances() {

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

    LOG(TRACE) << "Starting construction of the appliances " << m_Appliances_type;

    // Check that appliance type is valid.
    if(m_Appliances_type < 0 || m_Appliances_type > 1) {
        LOG(ERROR) << "Unknown Appliance Type : " << m_Appliances_type
                   << "Available types are 0,1. Set /allpix/extras/setApplianceType accordingly."
                   << "Quitting...";
        return;
    }

    // Check that we have some position vectors for the appliances.
    if(m_posVectorAppliances.empty()) {
        LOG(ERROR) << "You requested to build appliances, but no translation vector given."
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
    int id = 0;
    for(; aplItr != m_posVectorAppliances.end(); aplItr++) {
        string detname = (*aplItr).first;
        TString id_s = "_" + detname;

        // Translation vectors, with respect to the wrapper.
        // equals type-depending translation plus user given translation.
        TGeoTranslation* ApplTranslItr = new TGeoTranslation("ApplianceTransl" + id_s, 0., 0., 0.);
        ApplTranslItr->Add(&m_posVectorAppliances[detname]);
        ApplTranslItr->Add(ApplTransl);

        // Creation of the node.
        // The mother volume is the wrapper. It will rotate with the wrapper.
        TGeoVolume* Wrapper_log = gGeoManager->GetVolume(WrapperName + id_s);
        Wrapper_log->AddNode(Support_log, ++id, ApplTranslItr);

    } // end loop positions

    LOG(TRACE) << "Construction of the appliances successful.";
}

void GeometryBuilderTGeoModule::BuildTestStructure() {}

/*
  Create the materials and media.
*/
void GeometryBuilderTGeoModule::BuildMaterialsAndMedia() {

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
