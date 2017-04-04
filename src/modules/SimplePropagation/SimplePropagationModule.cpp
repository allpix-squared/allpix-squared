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

// temporary globals
XYZVector efield_from_map; // NOLINT

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
    // FIXME: modules should share random device?
    random_generator_.seed(std::random_device()());

    // set defaults for config variables
    config_.setDefault<double>("spatial_precision", 1e-10);
    config_.setDefault<double>("timestep_min", 0.005e-9);
    config_.setDefault<double>("timestep_max", 0.1e-9);
    config_.setDefault<unsigned int>("charge_per_step", 10);

    // FIXME: set fake linear electric field
    efield_from_map = XYZVector(0, 0, 100000);
}
SimplePropagationModule::~SimplePropagationModule() = default;

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
        // loop over all charges
        unsigned int electrons_remaining = deposit.getCharge();

        auto charge_per_step = config_.get<unsigned int>("charge_per_step");
        while(electrons_remaining > 0) {
            // define number of charges to be propagated and remove electrons of this step from the total
            if(charge_per_step > electrons_remaining) {
                charge_per_step = electrons_remaining;
            }
            electrons_remaining -= charge_per_step;

            // get position and propagate through sensor
            XYZVector position = deposit.getPosition(); // This is in a global frame!!!!!!!!!!!!

            // position = (*hitsCollection)[itr]->GetPosInLocalReferenceFrame();
            position.SetZ(position.z() + model_->getSensorSizeZ() / 2.);

            // propagate a single charge deposit
            position = propagate(position);

            LOG(DEBUG) << "charge propagated to " << position;
        }
    }
}

XYZVector SimplePropagationModule::propagate(const XYZVector& root_pos) {
    // create a runge kutta solver using the electric field as step function
    Eigen::Vector3d position(root_pos.x(), root_pos.y(), root_pos.z());

    // define a function to compute the electron mobility
    auto electron_mobility = [&](double efield_mag) {
        // variables for charge mobility
        auto Temperature = config_.get<double>("temperature");
        double Electron_Mobility =
            1.53e9 * TMath::Power(Temperature, -0.87) / (1.01 * TMath::Power(Temperature, 1.55)) * 1e-4;
        double Electron_Beta = 0.0257 * TMath::Power(Temperature, 0.66);
        double Electron_ec = 100 * 1.01 * TMath::Power(Temperature, 1.55);

        // calculate mobility in m2/V/s
        return Electron_Mobility *
               TMath::Power((1. + TMath::Power(efield_mag / Electron_ec, Electron_Beta)), -1.0 / Electron_Beta);
    };

    // build the runge kutta solver
    auto runge_kutta =
        make_runge_kutta(tableau::RK5,
                         [&](double, Eigen::Vector3d) -> Eigen::Vector3d {
                             // get the electric field and apply a drift velocity
                             auto efield =
                                 100. * Eigen::Vector3d(efield_from_map.x(), efield_from_map.y(), efield_from_map.z());
                             return -electron_mobility(efield.norm()) * (efield);
                         },
                         0.01 * 1e-9,
                         position);

    // continue until past the sensor
    // FIXME: we need to determine what would be a good time to stop
    while(position.z() > 0) {
        // do a runge kutta step
        auto step = runge_kutta.step();

        // get the current result and timestep
        double timestep = runge_kutta.getTimeStep();
        position = runge_kutta.getValue();

        // apply an extra diffusion step
        auto boltzmann_kT = 8.6173e-5 * config_.get<double>("temperature");
        XYZVector electric_field = 100. * efield_from_map;
        double D = boltzmann_kT * electron_mobility(TMath::Sqrt(electric_field.Mag2()));
        double Dwidth = TMath::Sqrt(2. * D * timestep);

        std::normal_distribution<double> gauss_distribution(0, Dwidth);
        Eigen::Vector3d diffusion;
        for(int i = 0; i < 3; ++i) {
            diffusion[i] = gauss_distribution(random_generator_);
        }
        runge_kutta.setValue(position + diffusion);

        // adapt step size to precision
        double uncertainty = step.error.norm();
        double target_spatial_precision = config_.get<double>("spatial_precision");
        if(model_->getSensorSizeZ() - position.z() < step.value.z() * 1.2) {
            timestep *= 0.7;
        } else {
            if(uncertainty > target_spatial_precision) {
                timestep *= 0.7;
            } else if(uncertainty < 0.5 * target_spatial_precision) {
                timestep *= 2;
            }
        }
        if(timestep > config_.get<double>("timestep_max")) {
            timestep = config_.get<double>("timestep_max");
        } else if(timestep < config_.get<double>("timestep_min")) {
            timestep = config_.get<double>("timestep_min");
        }
        runge_kutta.setTimeStep(timestep);
    }

    LOG(DEBUG) << "time : " << runge_kutta.getTime();

    position = runge_kutta.getValue();
    return XYZVector(position.x(), position.y(), position.z());
}
