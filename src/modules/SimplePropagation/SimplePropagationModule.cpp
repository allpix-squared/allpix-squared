/**
 * @author Paul Schuetze <paul.schuetze@desy.de>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SimplePropagationModule.hpp"

#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>

#include <Eigen/Core>

#include <Math/Vector3D.h>
#include <TMath.h>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/runge_kutta.h"

using namespace allpix;
using namespace ROOT::Math;

const std::string SimplePropagationModule::name = "SimplePropagation";

SimplePropagationModule::SimplePropagationModule(Configuration config,
                                                 Messenger* messenger,
                                                 std::shared_ptr<Detector> detector)
    : Module(detector), random_generator_(), config_(std::move(config)), detector_(std::move(detector)), model_(),
      deposits_message_(nullptr) {
    // get pixel detector model
    model_ = std::dynamic_pointer_cast<PixelDetectorModel>(detector_->getModel());

    // fetch deposits for single detector
    messenger->bindSingle(this, &SimplePropagationModule::deposits_message_);

    // seed the random generator
    // FIXME: should share random device
    random_generator_.seed(std::random_device()());

    // FIXME: temporary init variables
    init_variables();
}
SimplePropagationModule::~SimplePropagationModule() = default;

// temporary globals
XYZVector efield_from_map; // NOLINT
double Temperature;
double Electron_Mobility;
double Electron_Beta;
double Electron_ec;
double Boltzmann_kT;

double Timestep_max;
double Timestep_min;
double Target_Spatial_Precision;
unsigned int Electron_Scaling;

// run the propagation
void SimplePropagationModule::run() {
    // check model (NOTE: we need to check this constantly without an exception...)
    if(model_ == nullptr) {
        // FIXME: exception is more appropriate here
        LOG(CRITICAL) << "Detector " << detector_->getName()
                      << " is not a PixelDetectorModel: ignored as other types are currently unsupported!";
        return;
    }

    // propagate all deposits
    for(auto& deposit : deposits_message_->getData()) {
        // Loop over all electrons (do multiple electrons in one step)
        unsigned int createdElectronsRemaining = deposit.getCharge();

        while(createdElectronsRemaining > 0) {
            // Define number of electrons to be propagated and remove electrons of this step from the total
            unsigned int nElectrons;
            if(Electron_Scaling > createdElectronsRemaining) {
                nElectrons = createdElectronsRemaining;
            } else {
                nElectrons = Electron_Scaling;
            }

            createdElectronsRemaining -= nElectrons;

            // Get position and propagate through sensor
            XYZVector position = deposit.getPosition(); // This is in a global frame!!!!!!!!!!!!

            // position = (*hitsCollection)[itr]->GetPosInLocalReferenceFrame();
            position.SetZ(position.z() + model_->getSensorSizeZ() / 2.);

            // Propagate a single charge deposit
            position = propagation(position);

            LOG(DEBUG) << "charge propagated to " << position;

            // std::pair<int, int> endPixel;
            // endPixel.first = floor((position.x() + model_->getHalfSensorSizeX()) / model_->getPixelSizeX());
            // endPixel.second = floor((position.y() + model_->getHalfSensorSizeY()) / model_->getPixelSizeY());

            /*if(!chargeTrapped)
                pixelsContent[endPixel] += nElectrons;*/

        } // splitted electrons

    } // Charge collection
}

/* FIXME: temporary methods */
double MobilityElectron(double efield_mag);
Eigen::Vector3d ElectronSpeed(const Eigen::Vector3d& efield);
Eigen::Vector3d DiffusionStep(std::mt19937_64& random_generator, double timestep, const Eigen::Vector3d&);

double MobilityElectron(double efield_mag) {
    // calculate mobility in m2/V/s
    double mobility =
        Electron_Mobility * TMath::Power((1. + TMath::Power(efield_mag / Electron_ec, Electron_Beta)), -1.0 / Electron_Beta);
    // G4cout << "Mobility: " << mobility << G4endl;
    return mobility;
}
Eigen::Vector3d ElectronSpeed(const Eigen::Vector3d& efield) {
    double mobility = MobilityElectron(efield.norm());

    return mobility * (-efield);
}
Eigen::Vector3d DiffusionStep(std::mt19937_64& random_generator, double timestep, const Eigen::Vector3d&) {

    XYZVector electricField = 100. * efield_from_map;
    double D = Boltzmann_kT * MobilityElectron(TMath::Sqrt(electricField.Mag2()));
    double Dwidth = TMath::Sqrt(2. * D * timestep);

    std::normal_distribution<double> gauss(0, Dwidth);
    Eigen::Vector3d diffusionVector;
    for(int i = 0; i < 3; ++i) {
        diffusionVector[i] = gauss(random_generator);
    }

    return diffusionVector;
}
/* FIXME: end temporary methods */

// initialize variables
void SimplePropagationModule::init_variables() {
    // Set several variables (should be parameters to the config)
    Temperature = 273;

    // Variables for charge drift
    Electron_Mobility = 1.53e9 * TMath::Power(Temperature, -0.87) / (1.01 * TMath::Power(Temperature, 1.55)) * 1e-4;
    Electron_Beta = 0.0257 * TMath::Power(Temperature, 0.66);
    Electron_ec = 100 * 1.01 * TMath::Power(Temperature, 1.55);

    Boltzmann_kT = 8.6173e-5 * Temperature;

    // Variables for propagation speed
    Target_Spatial_Precision = 1e-10;
    Timestep_max = 0.1e-9;
    Timestep_min = 0.005e-9;
    Electron_Scaling = 10;

    // Fake linear electric field
    efield_from_map = XYZVector(0, 0, 10000);
}

XYZVector SimplePropagationModule::propagation(const XYZVector& root_pos) {
    // create a runge kutta solver using the electric field as step function
    Eigen::Vector3d position(root_pos.x(), root_pos.y(), root_pos.z());
    auto runge_kutta =
        make_runge_kutta(tableau::RK5,
                         [&](double, Eigen::Vector3d) {
                             auto electricField =
                                 100. * Eigen::Vector3d(efield_from_map.x(), efield_from_map.y(), efield_from_map.z());
                             return ElectronSpeed(electricField);
                         },
                         0.01 * 1e-9,
                         position);

    // continue until past the sensor
    // FIXME: we need to determine what would be a good time to stop
    while(position.z() > 0) {
        // do a runge kutta step
        auto step = runge_kutta.step();

        // get the current result
        position = runge_kutta.getValue();

        // apply an extra diffusion step
        position += DiffusionStep(random_generator_, runge_kutta.getTimeStep(), position);
        runge_kutta.setValue(position);

        // get the current step and error
        double dt = runge_kutta.getTimeStep();
        double uncertainty = step.error.norm();

        // adapt step size
        if(model_->getSensorSizeZ() - position.z() < step.value.z() * 1.2) {
            dt = dt * 0.7;
        } else {
            if(uncertainty > Target_Spatial_Precision) {
                dt *= 0.7;
            } else if(uncertainty < 0.5 * Target_Spatial_Precision) {
                dt *= 2;
            }
            if(dt > Timestep_max) {
                dt = Timestep_max;
            } else if(dt < Timestep_min) {
                dt = Timestep_min;
            }
        }
        runge_kutta.setTimeStep(dt);
    }

    position = runge_kutta.getValue();
    return XYZVector(position.x(), position.y(), position.z());
}
