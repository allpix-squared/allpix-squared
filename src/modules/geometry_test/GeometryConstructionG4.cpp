/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "GeometryConstructionG4.hpp"

//#include "G4SDManager.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4VisAttributes.hh"
#include "G4PVDivision.hh"
#include "G4VSolid.hh"
#include "G4ThreeVector.hh"
#include "G4Box.hh"
#include "G4PVPlacement.hh"
#include "G4UserLimits.hh"
#include "G4UnionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4Sphere.hh"
#include "G4Tubs.hh"
#include "G4PVParameterised.hh"

#include "CLHEP/Units/SystemOfUnits.h"

#include "BumpsParameterizationG4.hpp"
#include "DetectorModelG4.hpp"

#include "../../core/geometry/PixelDetectorModel.hpp"
#include "../../core/utils/log.h"

using namespace CLHEP;
using namespace std;

using namespace allpix;

G4VPhysicalVolume *GeometryConstructionG4::Construct(){
    // Materials
    // vacuum (FIXME: useless code, only here for later) 
    double z,a,density;
    G4Material *vacuum = new G4Material("Vacuum", z=1 , a=1.01*g/mole, density= 0.0001*g/cm3);
    
    // air
    G4NistManager * nistman = G4NistManager::Instance();
    G4Material *air = nistman->FindOrBuildMaterial("G4_AIR");
    
    // FIXME: stick to air as world material now
    world_material_ = air;
    
    LOG(DEBUG) << "Material of world: " << world_material_->GetName();
    
    /////////////////////////////////////////
    // The experimental Hall.  World
    G4VisAttributes * invisibleVisAtt = new G4VisAttributes(G4Color(1.0, 0.65, 0.0, 0.1));
    //G4VisAttributes invisibleVisAtt(G4Color(1.0, 0.65, 0.0, 0.1));
    invisibleVisAtt->SetVisibility(false);
    invisibleVisAtt->SetForceSolid(false);
    
    // Define the world
    G4Box* world_box = new G4Box("World",
                                   world_size_[0],
                                   world_size_[1],
                                   world_size_[2]);
    
    world_log_= new G4LogicalVolume(world_box,
                          world_material_,
                          "World",
                          0,0,0);
    world_log_->SetVisAttributes ( invisibleVisAtt );
    
    world_phys_ = new G4PVPlacement(0,
                        G4ThreeVector(0.,0.,0.),
                        world_log_,
                        "World",
                        0x0,
                        false,
                        0);
    
    //Build the pixel devices
    BuildPixelDevices();
    
    /*// Report absolute center of Si wafers
    if(!m_absolutePosSiWafer.empty()) {
        LOG(DEBUG) << "Absolute position of the Si wafers. center(x,y,z) : " ;
        vector<G4ThreeVector>::iterator itr = m_absolutePosSiWafer.begin();
        G4int cntr = 0;
        for( ; itr != m_absolutePosSiWafer.end() ; itr++) {
            
            LOG(DEBUG) << "   device [" << cntr << "] : "
            << (*itr).getX()/mm << " , "
            << (*itr).getY()/mm << " , "
            << (*itr).getZ()/mm << " [mm]"
            ;
            cntr++;

    }*/
    
    return world_phys_;
}

// WARNING: A DEFINE HERE THAT SHOULD PROBABLY BE A PARAMETER 
// NOTE: MULTIPLE ALERT HERE TO IDENTIFY PARTS THAT ARE NOT COPIED FROM ALLPIX 1
void GeometryConstructionG4::BuildPixelDevices() {
    G4NistManager * nistman = G4NistManager::Instance();

    // Air 
    G4Material *Air = nistman->FindOrBuildMaterial("G4_AIR");
    
    // Si
    G4Material * Silicon = nistman->FindOrBuildMaterial("G4_Si");

    // this is supposed to be epoxy // FIXME
    G4Material * Epoxy = nistman->FindOrBuildMaterial("G4_PLEXIGLASS");

    // Elements
    G4Element* Sn = new G4Element("Tin", "Sn", 50., 118.710*g/mole);
    G4Element* Pb = new G4Element("Lead","Pb", 82., 207.2*g/mole);

    // Materials from Combination, SnPb eutectic
    G4Material* Solder = new G4Material("Solder", 8.4*g/cm3, 2);
    Solder->AddElement(Sn,63);
    Solder->AddElement(Pb,37);

    ///////////////////////////////////////////////////////////////////////
    // vis attributes
    G4VisAttributes * pixelVisAtt= new G4VisAttributes(G4Color::Blue());
    pixelVisAtt->SetLineWidth(1);
    pixelVisAtt->SetForceSolid(true);
    //pixelVisAtt->SetForceWireframe(true);

    G4VisAttributes * BoxVisAtt= new G4VisAttributes(G4Color(0,1,1,1));
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

    G4VisAttributes * BumpBoxVisAtt = new G4VisAttributes(G4Color(0,1,0,1.0));
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

    ///////////////////////////////////////////////////////////////////////

    //vector<G4ThreeVector>::iterator posItr = pos.begin();
    //map<int, G4ThreeVector>::iterator posItr = m_posVector.begin();

    std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();

    ///////////////////////////////////////////////////////////
    // The pixel detector !
    //
    pair<G4String, G4String> wrapperName = make_pair("wrapper", "");
    pair<G4String, G4String> PCBName = make_pair("PCB", "");
    pair<G4String, G4String> BoxName = make_pair("Box", "");
    pair<G4String, G4String> CoverlayerName = make_pair("Coverlayer", "");
    pair<G4String, G4String> SliceName = make_pair("Slice", "");
    pair<G4String, G4String> GuardRingsExtName = make_pair("GuardRingsExt", "");
    pair<G4String, G4String> GuardRingsName = make_pair("GuardRings", "");
    pair<G4String, G4String> PixelName = make_pair("Pixel", "");
    pair<G4String, G4String> ChipName = make_pair("Chip", "");
    pair<G4String, G4String> SDName = make_pair("BoxSD", "");
    pair<G4String, G4String> BumpName = make_pair("Bump", "");
    pair<G4String, G4String> BumpBoxName = make_pair("BumpBox", "");

    // log an phys
    LOG(DEBUG) << "Building " << detectors.size() << " device(s) ..." ;

    // SD manager
    //G4SDManager *SDman = G4SDManager::GetSDMpointer();

    // User limits applied only to Si wafers.  Setting step.
#define PARAM_MAX_STEP_LENGTH DBL_MAX
    G4UserLimits *user_limits = new G4UserLimits(PARAM_MAX_STEP_LENGTH);

    auto detItr = detectors.begin();

    for( ; detItr != detectors.end() ; detItr++){
        // retrieve storage pointers
        std::shared_ptr<DetectorModelG4> model_g4 = std::make_shared<DetectorModelG4>();
        std::shared_ptr<PixelDetectorModel> model = std::dynamic_pointer_cast<PixelDetectorModel>((*detItr)->getModel());
        
        if(model == 0){
            LOG(WARNING) << "cannot build a G4 model for any non-pixel detectors yet... ignoring detector named " << (*detItr)->getName();
            continue;
        }
        
        // NOTE: create temporary internal integer
        // FIXME: this is not necessary unique! if this is necessary generate it internally!
        std::hash<std::string> hash_func;
        size_t temp_g4_id = hash_func((*detItr)->getName());
        
        LOG(DEBUG) << "start creating G4 detector " << (*detItr)->getName() << " (" << temp_g4_id << ")";
        
        /*------------------------------------ Solid definitions ----------------------------------------*/
        
        // replicated solids (same object/names everywhere)
        G4Box * Box_slice = new G4Box(SliceName.first,
                                      model->GetHalfPixelX(),
                                      model->GetHalfSensorY(),
                                      model->GetHalfSensorZ());
        
        G4Box * Box_pixel = new G4Box(PixelName.first,
                                      model->GetHalfPixelX(),
                                      model->GetHalfPixelY(),
                                      model->GetHalfPixelZ());
        
        //Bump itself
        G4UnionSolid * aBump = 0;
        G4Box * Bump_Box = 0;
        G4double bump_radius =0;
        G4double bump_dr =0;
        G4double bump_height=0;
        if ( model->GetBumpHeight() != 0.0 and model->GetHalfChipZ() != 0 ) {
            bump_radius =model->GetBumpRadius();
            bump_height = model->GetBumpHeight();
            bump_dr =  model->GetBumpDr();
            G4Sphere * aBump_Sphere = new G4Sphere(BumpName.first+"sphere",0,bump_radius,0,360*deg,0,360*deg);
            G4Tubs * aBump_Tube= new G4Tubs(BumpName.first+"Tube", 0., bump_radius-bump_dr, bump_height/2., 0., 360 *deg);
            aBump = new G4UnionSolid(BumpName.first,aBump_Sphere,aBump_Tube);
            
            //bumps containing volume
            
            Bump_Box = new G4Box(BumpBoxName.first,
                                 model->GetHalfSensorX(),
                                 model->GetHalfSensorY(),
                                 bump_height/2.);
        }
        
        // Build names
        std::string name = (*detItr)->getName();
        wrapperName.second = wrapperName.first + "_" + name;
        PCBName.second = PCBName.first + "_" + name;
        BoxName.second = BoxName.first + "_" + name;
        CoverlayerName.second = CoverlayerName.first + "_" + name;
        GuardRingsExtName.second = GuardRingsExtName.first  + "_" + name;
        GuardRingsName.second = GuardRingsName.first  + "_" + name;
        SliceName.second = BoxName.first + "_" + name;
        PixelName.second = BoxName.first + "_" + name;
        ChipName.second = ChipName.first + "_" + name;
        SDName.second = SDName.first + "_" + name; // BoxSD_XXX
        BumpName.second = BumpName.first + "_" + name; // BoxSD_XXX
        BumpBoxName.second = BumpBoxName.first + "_" + name; // BoxSD_XXX
        
        // Solids, I want different objects/names per medipix
        //  later on, they could be different
        G4Box * Box_box = new G4Box(BoxName.second,
                                    model->GetHalfSensorX(),
                                    model->GetHalfSensorY(),
                                    model->GetHalfSensorZ());
        
        G4Box * Chip_box = 0;
        if ( model->GetHalfChipZ() != 0 ) {
            // Chip box
            Chip_box = new G4Box(ChipName.second,
                                 model->GetHalfChipX(),
                                 model->GetHalfChipY(),
                                 model->GetHalfChipZ());
        }
        
        // If the coverlayer is requested.  It is forced to fit the sensor in X,Y.
        // The user can pick the thickness.
        /* G4Box * Coverlayer_box = nullptr;
        // ALERT: DISABLED!
        if ( model->IsCoverlayerON() ) {
            Coverlayer_box = new G4Box(
                CoverlayerName.second,
                model->GetHalfSensorX(),
                                       model->GetHalfSensorY(),
                                       model->GetHalfCoverlayerZ()
            );
        }*/
        
        // Guard rings will be GuardRingsExt - Box
        G4Box * Box_GuardRings_Ext = new G4Box(
            GuardRingsExtName.second,
            model->GetHalfSensorX() + (model->GetSensorExcessHRight() + model->GetSensorExcessHLeft()),
                                               model->GetHalfSensorY() + (model->GetSensorExcessHTop() + model->GetSensorExcessHBottom()),
                                               model->GetHalfSensorZ()); // same depth as the sensor
        
        G4VSolid * Solid_GuardRings =  new G4SubtractionSolid(GuardRingsName.second,
                                                              Box_GuardRings_Ext,
                                                              Box_box);
        
        
        G4Box* PCB_box = 0;
        if ( model->GetHalfPCBZ() != 0 ) {
            PCB_box = new G4Box(
                PCBName.second,
                model->GetHalfPCBX(),
                                model->GetHalfPCBY(),
                                model->GetHalfPCBZ());
        }
        
        // The wrapper might be enhanced when the user set up
        //  Appliances to the detector (extra layers, etc).
        G4double wrapperHX = model->GetHalfWrapperDX();
        G4double wrapperHY = model->GetHalfWrapperDY();
        G4double wrapperHZ = model->GetHalfWrapperDZ();
        
        // ALERT: DISABLED!
        // Apply the enhancement to the medipixes
        // We can have N medipixes and K enhancements, where K<=N.
        // For instance, for 2 medipixes.  We can have.
        // medipix 1 --> with enhancement
        // medipix 2 --> no enhancement
        /* if ( m_vectorWrapperEnhancement.findtemp_g4_id != m_vectorWrapperEnhancement.end() ) {
            wrapperHX += m_vectorWrapperEnhancement[*(*detItr)].x()/2.; // half
            wrapperHY += m_vectorWrapperEnhancement[*(*detItr)].y()/2.;
            wrapperHZ += m_vectorWrapperEnhancement[*(*detItr)].z()/2.;
            // temp test
            //wrapperDZ += 10000*um;
        } */
        
        LOG(DEBUG) << "Wrapper Dimensions [mm] : " << wrapperHX/mm << " " << wrapperHY/mm << " " << wrapperHZ/mm;
        
        G4Box* wrapper_box = new G4Box(wrapperName.second,
                                       2.*wrapperHX,
                                       2.*wrapperHY,
                                       2.*wrapperHZ);
        
        //G4RotationMatrix yRot45deg;   // Rotates X and Z axes only
        //yRot45deg.rotateY(M_PI/4.*rad);
        //G4ThreeVector  translation(0, 0, 50*mm);
        //G4UnionSolid  wrapper_box_union("wrapper_box_union",
        //                                wrapper_box, wrapper_box, &yRot45deg, translation);
        
        /*------------------------------------ Logical and physical volumes definitions ----------------------------------------*/
        
        ///////////////////////////////////////////////////////////
        // wrapper
        model_g4->wrapper_log = new G4LogicalVolume(wrapper_box,
                                                       world_material_,
                                                       wrapperName.second+"_log");
        model_g4->wrapper_log->SetVisAttributes(wrapperVisAtt);
        
        
        //ALERT: DISABLED (but not used anyway?)
        //G4double sensorOffsetX = model->GetSensorXOffset();
        //G4double sensorOffsetY = model->GetSensorYOffset();
        
        // WARNING: get a proper geometry lib
        std::tuple<double, double, double> pos_tup = (*detItr)->getPosition();
        G4ThreeVector posWrapper;
        posWrapper[0] = std::get<0>(pos_tup);
        posWrapper[1] = std::get<1>(pos_tup);
        posWrapper[2] = std::get<2>(pos_tup);
        
        // ALERT: DISABLED!
        // Apply position Offset for the wrapper due to the enhancement
        /*if ( m_vectorWrapperEnhancement.findtemp_g4_id != m_vectorWrapperEnhancement.end() ) {
            posWrapper.setX(posWrapper.x() + m_vectorWrapperEnhancement[*(*detItr)].x()/2.);
            posWrapper.setY(posWrapper.y() + m_vectorWrapperEnhancement[*(*detItr)].y()/2.);
            posWrapper.setZ(posWrapper.z() + m_vectorWrapperEnhancement[*(*detItr)].z()/2.);
        } else {*/
            //posWrapper.setX(posWrapper.x() - sensorOffsetX );
            //posWrapper.setY(posWrapper.y() - sensorOffsetY );
            posWrapper.setX(posWrapper.x() );
            posWrapper.setY(posWrapper.y() );
            posWrapper.setZ(posWrapper.z() );
        //}
        
        
        //G4Transform3D transform( *m_rotVector[temp_g4_id], posWrapper );
        
        // WARNING: get a proper geometry lib
        std::tuple<double, double, double> rot_tup = (*detItr)->getOrientation();
        G4RotationMatrix *rotWrapper = new G4RotationMatrix(std::get<0>(rot_tup), std::get<1>(rot_tup), std::get<2>(rot_tup));
        /*rotWrapper[0] = std::get<0>(rot_tup);
        rotWrapper[1] = std::get<1>(rot_tup);
        rotWrapper[2] = std::get<2>(rot_tup);*/
            
        // starting at user position --> vector pos
        model_g4->wrapper_phys = new G4PVPlacement(
            //transform,
            rotWrapper,
            posWrapper,
            model_g4->wrapper_log,
            wrapperName.second+"_phys",
            world_log_,
            false,
            temp_g4_id, // copy number
            true);
        
        // Apply a translation to the wrapper first
        //G4Transform3D tA = G4TranslateX3D( -1*model->GetSensorXOffset() );
        //G4Transform3D tB = G4TranslateY3D( -1*model->GetSensorYOffset() );
        //G4Transform3D transform = tA * tB;
        //model_g4->wrapper_phys->SetTranslation( transform.getTranslation() );
        
        ///////////////////////////////////////////////////////////
        // PCB
        // The PCB is placed respect to the wrapper.
        // Needs to be pushed -half Si wafer in z direction
        model_g4->PCB_log = 0;
        if(model->GetHalfPCBZ()!=0){
            
            model_g4->PCB_log = new G4LogicalVolume(PCB_box,
                                                       Epoxy,
                                                       PCBName.second+"_log");
            model_g4->PCB_log->SetVisAttributes(pcbVisAtt);
            
        }
        
        if ( model->GetHalfChipZ()!=0 ) {
            
            ///////////////////////////////////////////////////////////
            // Chip
            // The Si wafer is placed respect to the wrapper.
            // Needs to be pushed -half Si wafer in z direction
            
            model_g4->chip_log = new G4LogicalVolume(Chip_box,
                                                        Silicon,
                                                        ChipName.second+"_log");
            model_g4->chip_log->SetVisAttributes(ChipVisAtt);
            
            if ( model->GetBumpHeight()!=0.0 ) {
                ///////////////////////////////////////////////////////////
                // Bumps
                model_g4->bumps_log = new G4LogicalVolume(Bump_Box,
                                                             Air,
                                                             BumpBoxName.second+"_log");
                model_g4->bumps_log->SetVisAttributes(BumpBoxVisAtt);
            }
            
            
        }
        
        
        ///////////////////////////////////////////////////////////
        // Device
        // The Si wafer is placed respect to the wrapper.
        // Needs to be pushed -half Si wafer in z direction
        
        model_g4->box_log = new G4LogicalVolume(Box_box,
                                                   Silicon,
                                                   BoxName.second+"_log");
        
        model_g4->box_log->SetVisAttributes(BoxVisAtt);
        
        // positions
        G4ThreeVector posCoverlayer(0,0,0);
        //G4ThreeVector posDevice(sensorOffsetX,sensorOffsetY,0);
        G4ThreeVector posDevice(0,0,0);
        G4ThreeVector posBumps(0,0,0);
        G4ThreeVector posChip(0,0,0);
        G4ThreeVector posPCB(0,0,0);
        
        
        // ALERT: DISABLED!
        /*if ( model->IsCoverlayerON() ) {
            
            posCoverlayer.setX( posDevice.x() );
            posCoverlayer.setY( posDevice.y() );
            posCoverlayer.setZ(
                posDevice.z()
                //wrapperHZ
                -model->GetHalfSensorZ()
                -model->GetHalfCoverlayerZ()
            );
        }*/
        
        // Apply position Offset for the detector due to the enhancement
        // ALERT: DISABLED
        /*if ( m_vectorWrapperEnhancement.findtemp_g4_id != m_vectorWrapperEnhancement.end() ) {
            
            posDevice.setX(posDevice.x() - m_vectorWrapperEnhancement[*(*detItr)].x()/2.);
            posDevice.setY(posDevice.y() - m_vectorWrapperEnhancement[*(*detItr)].y()/2.);
            posDevice.setZ(posDevice.z() - m_vectorWrapperEnhancement[*(*detItr)].z()/2.);
            //wrapperHZ
            //- 2.*model->GetHalfCoverlayerZ()
            //- model->GetHalfSensorZ()
            //- m_vectorWrapperEnhancement[*(*detItr)].z()/2.
            //);
            //posDevice.z() - m_vectorWrapperEnhancement[*(*detItr)].z()/2.);
        } else {*/
            posDevice.setX(posDevice.x() );
            posDevice.setY(posDevice.y() );
            posDevice.setZ(posDevice.z() );
            //wrapperHZ
            //- 2.*model->GetHalfCoverlayerZ()
            //- model->GetHalfSensorZ()
            //);
        //}
        
        ///////////////////////////////////////////////////////////
        // Coverlayer if requested (typically made of Al, but user configurable)
        
        // ALERT: DISABLED
        /*if ( model->IsCoverlayerON() ) {
            
            // Find out about the material that the user requested
            G4Material * covermat = nistman->FindOrBuildMaterial( model->GetCoverlayerMat() );
            
            // If this is an inexistent choice, them force Aluminum
            if ( covermat == nullptr ) {
                covermat = nistman->FindOrBuildMaterial( "G4_Al" );
            }
            
            m_Coverlayer_log[temp_g4_id] = new G4LogicalVolume( Coverlayer_box,
                                                               covermat,
                                                               CoverlayerName.second+"_log");
            
            m_Coverlayer_log[temp_g4_id]->SetVisAttributes(CoverlayerVisAtt);
        }*/
        
        // Calculation of position of the different physical volumes
        if ( model->GetHalfChipZ() != 0 ) {
            
            posBumps.setX( posDevice.x() );
            posBumps.setY( posDevice.y() );
            posBumps.setZ( posDevice.z()
            //wrapperHZ
            - model->GetHalfSensorZ()
            - 2.*model->GetHalfCoverlayerZ()
            - (bump_height/2.)
            );
            
            //posDevice.z() -
            //model->GetHalfSensorZ() +
            //model->GetChipZOffset() -
            //bump_height/2
            //);
            
            posChip.setX( posDevice.x() + model->GetChipXOffset() );
            posChip.setY( posDevice.y() + model->GetChipYOffset() );
            posChip.setZ( posDevice.z()
            //wrapperHZ
            - model->GetHalfSensorZ()
            - 2.*model->GetHalfCoverlayerZ()
            - bump_height
            - model->GetHalfChipZ()
            + model->GetChipZOffset()
            );
            
            //posDevice.z() -
            //model->GetHalfSensorZ() -
            //model->GetHalfChipZ() +
            //model->GetChipZOffset() -
            //bump_height
            //);
            
        } else {
            // Make sure no offset because of bumps for PCB is calculated if chip is not included
            bump_height = 0;
        }
        
        //posPCB.setX( 0 ); //- 1.*model->GetSensorXOffset() );
        //posPCB.setY( 0 ); //- 1.*model->GetSensorYOffset() );
        posPCB.setX(posDevice.x()-1*model->GetSensorXOffset());
        posPCB.setY(posDevice.y()-1*model->GetSensorYOffset());
        posPCB.setZ(posDevice.z()
        //wrapperHZ
        - model->GetHalfSensorZ()
        - 2.*model->GetHalfCoverlayerZ()
        - bump_height
        - 2.*model->GetHalfChipZ()
        - model->GetHalfPCBZ()
        );
        
        //posDevice.z() -
        //model->GetHalfSensorZ() -
        //2*model->GetHalfChipZ() -
        //model->GetHalfPCBZ() -
        //bump_height
        //);
        
        LOG(DEBUG) << "- Coverlayer position  : " << posCoverlayer ;
        LOG(DEBUG) << "- Sensor position      : " << posDevice ;
        LOG(DEBUG) << "- Bumps position       : " << posBumps ;
        LOG(DEBUG) << "- Chip position        : " << posChip ;
        LOG(DEBUG) << "- PCB position         : " << posPCB ;
        
        
        /*------------------------------------ Physical placement  ----------------------------------------*/
        
        model_g4->PCB_phys = 0;
        if(model->GetHalfPCBZ()!=0){
            
            model_g4->PCB_phys = new G4PVPlacement(
                0,
                posPCB,
                model_g4->PCB_log,
                PCBName.second+"_phys",
                model_g4->wrapper_log, // mother log
                false,
                temp_g4_id,
                true); // copy number
        }
        if(model->GetHalfChipZ()!=0){
            
            model_g4->chip_phys = new G4PVPlacement(0,
                                                    posChip,
                                                    model_g4->chip_log,
                                                    ChipName.second+"_phys",
                                                    model_g4->wrapper_log, // mother log
                                                    false,
                                                    temp_g4_id, // copy number
                                                    true); // check overlap
            if(model->GetBumpHeight()!=0.0){
                model_g4->bumps_phys = new G4PVPlacement(0,
                                                    posBumps,
                                                    model_g4->bumps_log,
                                                    BumpBoxName.second+"_phys",
                                                    model_g4->wrapper_log, // mother log
                                                    false,
                                                    temp_g4_id, // copy number
                                                    true); // check overlap
            };
        }
        
        model_g4->box_phys = new G4PVPlacement(
                                                    0,
                                                    posDevice,
                                                    model_g4->box_log,
                                                    BoxName.second+"_phys",
                                                    model_g4->wrapper_log, // mother log
                                                    false,
                                                    temp_g4_id, // copy number
                                                    true); // check overlap
        
        // coverlayer
        // ALERT: DISABLED
        /*if ( model->IsCoverlayerON() ) {
            
            m_Coverlayer_phys[temp_g4_id] = new G4PVPlacement(
                0,
                posCoverlayer,
                m_Coverlayer_log[temp_g4_id],
                                                             CoverlayerName.second+"_phys",
                                                             model_g4->wrapper_log, // mother log
                                                             false,
                                                             temp_g4_id, // copy number
                                                             true); // check overlap
            
        }*/
        
        ///////////////////////////////////////////////////////////
        // bumps
        
        if ( model->GetHalfChipZ() != 0 ) {
            
            model_g4->bumps_cell_log = new G4LogicalVolume(aBump,Solder,BumpBoxName.second+"_log" );
            model_g4->bumps_cell_log->SetVisAttributes(BumpVisAtt);
            
            //		m_Bumps_Slice_log[temp_g4_id] = new G4LogicalVolume(Bump_Slice_Box,Air,BumpSliceName.second+"_log");
            //		m_Bumps_Slice_log[temp_g4_id]->SetVisAttributes(BumpSliceVisAtt);
            
            
            model_g4->parameterization_ = new BumpsParameterizationG4( model );
            G4int NPixTot = model->GetNPixelsX()*model->GetNPixelsY();
            new G4PVParameterised(BumpName.second+"phys",
                                  model_g4->bumps_cell_log,        // logical volume
                                  model_g4->bumps_log,             // mother volume
                                  kUndefined,                         // axis
                                  NPixTot,                            // replicas
                                  model_g4->parameterization_);       // G4VPVParameterisation
        }
        
        
        ///////////////////////////////////////////////////////////
        // slices and pixels
        model_g4->slice_log = new G4LogicalVolume(Box_slice,
                                                     Silicon,
                                                     SliceName.second); // 0,0,0);
                                                     //model_g4->slice_log->SetUserLimits(ulim);
                                                     model_g4->pixel_log = new G4LogicalVolume(Box_pixel,
                                                                                                  Silicon,
                                                                                                  PixelName.second); // 0,0,0);
                                                                                                  
        if (user_limits) model_g4->pixel_log->SetUserLimits(user_limits);
        
        // divide in slices
        new G4PVDivision(
            SliceName.second,
            model_g4->slice_log,
            model_g4->box_log,
            kXAxis,
            //model->GetPixelX(),
            model->GetNPixelsX(),
            0); // offset
        
        new G4PVDivision(
            PixelName.second,
            model_g4->pixel_log,
            model_g4->slice_log,
            kYAxis,
            //model->GetPixelY(),
            model->GetNPixelsY(),
            0); // offset
        
        ///////////////////////////////////////////////////////////
        // Guard rings and excess area
        model_g4->guard_rings_log = new G4LogicalVolume(Solid_GuardRings,
                                                        Silicon,
                                                        GuardRingsName.second+"_log");
        model_g4->guard_rings_log->SetVisAttributes(guardRingsVisAtt);
        model_g4->guard_rings_phys = new G4PVPlacement(0,
                                                        posDevice,
                                                        model_g4->guard_rings_log,
                                                        GuardRingsName.second+"_phys",
                                                        model_g4->wrapper_log, // mother log
                                                        false,
                                                        0,//temp_g4_id, // copy number
                                                        true); // check overlap
        
        // Find out where the center of the Silicon has been placed
        // for user information
        
        //ALERT: DISABLED
        /*m_absolutePosSiWafer.push_back(posWrapper + posDevice); */
        
        //LOG(DEBUG) << posWrapper << " " << posDevice << endl;
        
        // SD --> pixels !
        // AllPixTrackerSD instance needs to know the absolute position of
        //  the device and rotation.  'm_absolutePosSiWafer' and 'rot' are
        //  the good figures.
        
        // ALERT: DISABLED
        // FIXME: this should move to the deposition module
        /*G4ThreeVector origin(0,0,0);
        
        AllPixTrackerSD * aTrackerSD = new AllPixTrackerSD( SDName.second,
                                                            (posWrapper + posDevice),
                                                            posDevice,
                                                            model,
                                                            m_rotVector[temp_g4_id] );
        
        SDman->AddNewDetector( aTrackerSD );
        model_g4->pixel_log->SetSensitiveDetector( aTrackerSD );*/
        
         // ALERT: DISABLED
        // FIXME: most of this should not be defined here (should go to the deposition module)
        
        /*
        // Store the hit Collection name in the geometry
        model->SetHitsCollectionName( aTrackerSD->GetHitsCollectionName() );
        
        // Read electric field from file if necessary
        if(m_EFieldFiles.counttemp_g4_id>0) model->SetEFieldMap(m_EFieldFiles[temp_g4_id]);
        
        if(m_temperatures.counttemp_g4_id>0)
        {
            model->SetTemperature(m_temperatures[temp_g4_id]);
        }else{
            model->SetTemperature(300.);
        }
        model->SetFlux(m_fluxes[temp_g4_id]);
        
        model->SetMagField(m_magField_cartesian);
        */
        
        // add this geant4 model to the detector
        (*detItr)->setExternalModel(model_g4);
        
        LOG(DEBUG) << "detector " << (*detItr)->getName() << " ... done" ;
        
        //model_g4->wrapper_phys->SetTranslation(posWrapper-posDevice);
        
        //model_g4->wrapper_phys->SetRotation( m_rotVector[temp_g4_id] );
                                                                                                
    }
}
