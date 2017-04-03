/**
 * @author Paul Schuetze <paul.schuetze@desy.de>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SimplePropagationModule.hpp"

#include <string>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

using namespace allpix;

const std::string SimplePropagationModule::name = "SimplePropagation";

SimplePropagationModule::SimplePropagationModule(Configuration config,
                                                 Messenger* messenger,
                                                 std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(std::move(detector)), deposits_message_(nullptr) {
    messenger->bindSingle(this, &SimplePropagationModule::deposits_message_);
}
SimplePropagationModule::~SimplePropagationModule() = default;

// run the deposition
void SimplePropagationModule::run() {}

/*void SimplePropagationModule::InitVariables(){

    // gD contains info on sensor and surroundings (bfield, temperature, ...)
    gD = GetDetectorGeoDscPtr();

    bfield = gD->GetMagField();
    detectorThickness = gD->GetSensorZ();
    Temperature = gD->GetTemperature();
    flux = gD->GetFlux(); // 1/cm^2

    PixelSizeX = gD->GetPixelX();
    PixelSizeY = gD->GetPixelY();
    NPixelX = gD->GetNPixelsX();
    NPixelY = gD->GetNPixelsY();
    SensorPosX = gD->GetSensorXOffset();
    SensorPosY = gD->GetSensorYOffset();
    SensorHalfSizeX = gD->GetHalfSensorX();
    SensorHalfSizeY = gD->GetHalfSensorY();

    ///////////////////////////////////////////////////
    // Silicon electron and hole transport constants //
    ///////////////////////////////////////////////////

    //// Unit for charge in FEIX average e/h pair creation energy in Silicon
    elec = 3.64*eV;

    // Variables for charge drift
    Electron_Mobility = 1.53e9*TMath::Power(Temperature, -0.87)/(1.01*TMath::Power(Temperature, 1.55))*1e-4; // mu0 from
pixelav, m2/volt/s
    Electron_Beta = 0.0257*TMath::Power(Temperature, 0.66); // beta from pixelav

    Electron_HallFactor = 1.12;
    Electron_ec = 100*1.01 * TMath::Power(Temperature, 1.55); // ec from pixelav

    Boltzmann_kT = 8.6173e-5*Temperature; // eV
    // Boltzmann_kT = 1.38e-23*Temperature; // J

    Target_Spatial_Precision = 1e-10;
    Timestep_max = 0.1e-9;
    Timestep_min = 0.005e-9;

    Electron_Scaling = 10;

    // Variables for Smearing and Digitizing
    threshold = gD->GetThreshold();
    gainFactor = 1.04;
    gaussNoise = gD->GetChipNoise();
    thresholdSmear = 100.;
    ADCSmear = 2.;
    gaincalParameters = new G4double[5];
    gaincalParameters[0] = 0.999745;
    gaincalParameters[1] = 287117.6;
    gaincalParameters[2] = 4853.0;
    gaincalParameters[3] = -257.8;
    gaincalParameters[4] = 227.6;

    // Variables for Trapping
    Electron_Trap_beta0 = 5.65e-7; // cm2/s
    Electron_Trap_kappa = -0.86;
    Electron_Trap_T0 = 263; // K
    Electron_Trap_TauNoFluence = 1.;

    if(flux > 0.){
        Electron_Trap_TauEff = 1./(flux * Electron_Trap_beta0 *
TMath::Power((Temperature/Electron_Trap_T0),Electron_Trap_kappa));
    }else{
        Electron_Trap_TauEff = Electron_Trap_TauNoFluence;
    }

}

void SimplePropagationModule::Digitize(){

    // create the digits collection
    m_digitsCollection = new AllPixCMSp1DigitsCollection("AllPixCMSp1Digitizer", collectionName[0] );

    // get the digiManager
    G4DigiManager * digiMan = G4DigiManager::GetDMpointer();

    // Get the hit collection ID
    G4int hcID = digiMan->GetHitsCollectionID(m_hitsColName[0]);

    // And fetch the Hits Collection
    AllPixTrackerHitsCollection * hitsCollection = 0;
    hitsCollection = (AllPixTrackerHitsCollection*)(digiMan->GetHitsCollection(hcID));

    // temporary data structure
    map<pair<G4int, G4int>, G4double > pixelsContent;
    pair<G4int, G4int> tempPixel;
    pair<G4int, G4int> endPixel;

    G4double createdElectronsStep = 0;
    G4double createdElectronsRemaining = 0;
    G4double nElectrons = 0;

    G4double drifttime;
    G4bool chargeTrapped;

    G4int nEntries = hitsCollection->entries();

    G4ThreeVector position;

    for(G4int itr  = 0 ; itr < nEntries ; itr++) {

        // Calculate number of electrons
        createdElectronsStep =
1e6*eV*CLHEP::RandGauss::shoot((*hitsCollection)[itr]->GetEdep()/elec,TMath::Sqrt((*hitsCollection)[itr]->GetEdep()/elec)*0.118);

        tempPixel.first  = (*hitsCollection)[itr]->GetPixelNbX();
        tempPixel.second = (*hitsCollection)[itr]->GetPixelNbY();

        // Loop over all electrons (do (Electron_Scaling) electrons in one step)
        createdElectronsRemaining = createdElectronsStep;
        while(createdElectronsRemaining > 0.){

            // Define number of electrons to be propagated and remove electrons of this step from the total
            if(Electron_Scaling > createdElectronsRemaining){
                nElectrons = createdElectronsRemaining;
            }else{
                nElectrons = Electron_Scaling;
            }

            createdElectronsRemaining -= nElectrons;

            // Get Position and propagate through sensor
            position = (*hitsCollection)[itr]->GetPos(); // This is in a global frame!!!!!!!!!!!!

            position = (*hitsCollection)[itr]->GetPosInLocalReferenceFrame();
            position[2] += detectorThickness/2.;

            // G4cout << position << G4endl;
            Propagation(position, drifttime, chargeTrapped);
            endPixel.first = floor((position.x()+SensorHalfSizeX)/PixelSizeX);
            endPixel.second = floor((position.y()+SensorHalfSizeY)/PixelSizeY);

            if(!chargeTrapped) pixelsContent[endPixel] += nElectrons;

        } // splitted electrons

    } // Charge collection
}



vector<G4double>  SimplePropagationModule::RKF5Integration(G4ThreeVector position, G4double dt)
{
    // This function transport using Euler integration, for field (Ex,Ey,Ez),
    // considered constant over time dt. The movement equation are those
    // of charges in semi-conductors, sx= mu*E*dt;;
    G4ThreeVector deltaposition;

    G4ThreeVector electricField;
    G4ThreeVector temppos;

    G4ThreeVector k1, k2, k3, k4, k5, k6;

    electricField = 100.*gD->GetEFieldFromMap(position);
    k1 = (ElectronSpeed(electricField)*dt);

    // G4cout << "z: " << position[2] << "\t\t" << ElectronSpeed(electricField) << G4endl;
    // G4cout << "z: " << position[2] << "\t\t" << electricField << G4endl;

    // G4cout << "Field: " << electricField << G4endl;
    // G4cout << "Speed: " << ElectronSpeed(electricField) << G4endl;

    temppos = position + (1./4.)*k1;
    electricField = 100.*gD->GetEFieldFromMap(temppos);
    k2 = (ElectronSpeed(electricField)*dt);

    temppos = position + (3./32.)*k1 + (9./32.)*k2;
    electricField = 100.*gD->GetEFieldFromMap(temppos);
    k3 = (ElectronSpeed(electricField)*dt);

    temppos = position + (1932./2197)*k1 + (-7200./2197)*k2 + (7296./2197)*k3;
    electricField = 100.*gD->GetEFieldFromMap(temppos);
    k4 = (ElectronSpeed(electricField)*dt);

    temppos = position + (439./216)*k1 + (-8)*k2 + (3680./513)*k3 + (-845./4104)*k4;
    electricField = 100.*gD->GetEFieldFromMap(temppos);
    k5 = (ElectronSpeed(electricField)*dt);

    temppos = position + (-8./27)*k1 + (2)*k2 + (-3544./2565)*k3 + (1859./4104)*k4 + (-11./40)*k5;
    electricField = 100.*gD->GetEFieldFromMap(temppos);
    k6 = (ElectronSpeed(electricField)*dt);

    deltaposition = (16./135)*k1 + (0.)*k2 + (6656./12825)*k3 + (28561./56430)*k4 + (-9./50)*k5 + (2./55)*k6;

    G4ThreeVector error;
    error = (1./360)*k1 + (0.)*k2 + (-128./4275)*k3 + (-2197./75240)*k4 + (1./50)*k5 + (2./55)*k6;

    G4double Erreur;
    Erreur=error.mag();

    vector<G4double> deltapoint(4);
    deltapoint[0]=deltaposition[0];
    deltapoint[1]=deltaposition[1];
    deltapoint[2]=deltaposition[2];
    deltapoint[3]=(Erreur);

    return deltapoint;

}

G4double SimplePropagationModule::MobilityElectron(const G4ThreeVector efield){

    // calculate mobility in m2/V/s

    G4double mobility = Electron_Mobility * TMath::Power((1.+
TMath::Power(efield.mag()/Electron_ec,Electron_Beta)),-1.0/Electron_Beta);
    // G4cout << "Mobility: " << mobility << G4endl;
    return mobility;

}

G4ThreeVector SimplePropagationModule::ElectronSpeed(const G4ThreeVector efield){

    G4ThreeVector term[3];
    G4double mobility = MobilityElectron(efield);

    G4double rnorm = 1. + mobility*mobility*Electron_HallFactor*Electron_HallFactor*bfield.dot(bfield);

    term[0] = -efield;
    term[1] = mobility*Electron_HallFactor*efield.cross(bfield);
    term[2] = -mobility*mobility*Electron_HallFactor*Electron_HallFactor*efield.dot(bfield)*bfield;

    G4ThreeVector speed = mobility*(term[0]+term[1]+term[2])/rnorm;

    return speed;

}

G4ThreeVector SimplePropagationModule::DiffusionStep(const G4double timestep, const G4ThreeVector position){

    G4ThreeVector diffusionVector;

    G4ThreeVector electricField = 100.*gD->GetEFieldFromMap(position);
    G4double D = Boltzmann_kT*MobilityElectron(electricField);
    G4double Dwidth = TMath::Sqrt(2.*D*timestep);

    for (size_t i = 0; i < 3; i++) {
        diffusionVector[i] = CLHEP::RandGauss::shoot(0.,Dwidth)*um;
    }

    // TMath::Sqrt(2.*D*timestep);
    return diffusionVector;

}

void SimplePropagationModule::SetDt(G4double& dt, const G4double uncertainty, const G4double z, const G4double dz){

    G4double dt_init = dt;

    if(uncertainty > Target_Spatial_Precision){dt *= 0.7;}
    if(uncertainty < 0.5*Target_Spatial_Precision){dt *= 2;}

    if(dt > Timestep_max){dt = Timestep_max;}
    if(dt < Timestep_min){dt = Timestep_min;}

    if(detectorThickness-z < dz*1.2){dt = dt_init*0.7;}

}

G4double SimplePropagationModule::Propagation(G4ThreeVector& pos, G4double& drifttime, G4bool& trapped){

    vector<G4double> deltapoint(4);

    drifttime = 0.;
    G4double dt = 0.01*1e-9;

    G4double trappingTime = -Electron_Trap_TauEff*log(CLHEP::RandFlat::shoot());

    trapped = false;

    G4int nsteps=0;

    while(abs(pos[2]) < detectorThickness/1000./2.)
    {

        if(drifttime > trappingTime){
            trapped = true;
            break;
        }

        deltapoint = RKF5Integration(pos,dt);
        for (size_t i = 0; i < 3; i++) {pos[i]+=deltapoint[i]/nm*um;}
        drifttime += dt;


        pos += DiffusionStep(dt, pos);


        // Adapt step size
        SetDt(dt, deltapoint[3], pos[2], deltapoint[2]/nm*um);

        nsteps++;
    }
    // if(trapped) G4cout << "Charge was trapped." << G4endl;
    // G4cout << "Endposition: " << pos << G4endl;
    // G4cout << "Total drift time in ns: " << drifttime*1e9 << G4endl;
    // G4cout << "Step count: " << nsteps << G4endl;

    return drifttime;

}*/
