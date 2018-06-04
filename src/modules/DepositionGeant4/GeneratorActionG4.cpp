/**
 * @file
 * @brief Implements the particle generator
 * @remark Based on code from John Idarraga
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "GeneratorActionG4.hpp"

#include <limits>
#include <memory>

#include <G4Event.hh>
#include <G4GeneralParticleSource.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4RunManager.hh>
#include <G4UImanager.hh>
#include <TRandom3.h>
#include <core/module/exceptions.h>

#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/geant4.h"

using namespace allpix;

GeneratorActionG4::GeneratorActionG4(const Configuration& config)
    : particle_source_(std::make_unique<G4GeneralParticleSource>()) {

    // Set verbosity of source to off
    particle_source_->SetVerbosity(0);

    // Get source specific parameters
    auto single_source = particle_source_->GetCurrentSource();

    // Set the centre coordinate of the source
    G4ThreeVector source_pos = config.get<G4ThreeVector>("source_position");
    single_source->GetPosDist()->SetCentreCoords(source_pos);

    if(config.get<std::string>("source_type") == "macro") {

        LOG(INFO) << "Using user macro for particle source.";

        // Get the UI commander
        G4UImanager* UI = G4UImanager::GetUIpointer();

        // Execute the user's macro
        std::ifstream file(config.getPath("file_name", true));
        std::string line;
        while(std::getline(file, line)) {
            // Check for the "/gps/" pattern in the line:
            if(!line.empty()) {
                if(line.substr(0, 13) == "/gps/position" || line.substr(0, 15) == "/gps/pos/centre") {
                    throw ModuleError(
                        "The source position must be defined in the main configuration file, not in the macro.");
                } else if(line.substr(0, 11) == "/gps/number") {
                    throw ModuleError(
                        "The number of particles must be defined in the main configuration file, not in the macro.");
                } else if(line.substr(0, 13) == "/gps/particle" || line.substr(0, 10) == "/gps/hist/" ||
                          line.substr(0, 9) == "/gps/ang/" || line.substr(0, 9) == "/gps/pos/" ||
                          line.substr(0, 9) == "/gps/ene/" || line.at(0) == '#') {
                    UI->ApplyCommand(line);
                } else {
                    LOG(WARNING) << "Ignoring Geant4 macro command: \"" + line + "\" - not related to particle source.";
                }
            }
        }

    } else {

        // Set position and direction parameters according to shape
        if(config.get<std::string>("source_type") == "beam") {

            // Set position parameters
            single_source->GetPosDist()->SetPosDisType("Beam");
            single_source->GetPosDist()->SetBeamSigmaInR(config.get<double>("beam_size", 0));

            // Set angle distribution parameters
            single_source->GetAngDist()->SetAngDistType("beam2d");
            single_source->GetAngDist()->DefineAngRefAxes("angref1", G4ThreeVector(-1., 0, 0));
            G4TwoVector divergence = config.get<G4TwoVector>("beam_divergence", G4TwoVector(0., 0.));
            single_source->GetAngDist()->SetBeamSigmaInAngX(divergence.x());
            single_source->GetAngDist()->SetBeamSigmaInAngY(divergence.y());
            G4ThreeVector direction = config.get<G4ThreeVector>("beam_direction");
            if(fabs(direction.mag() - 1.0) > std::numeric_limits<double>::epsilon()) {
                LOG(WARNING) << "Momentum direction is not a unit vector: magnitude is ignored";
            }
            single_source->GetAngDist()->SetParticleMomentumDirection(direction);

        } else if(config.get<std::string>("source_type") == "sphere") {

            // Set position parameters
            single_source->GetPosDist()->SetPosDisType("Volume");
            single_source->GetPosDist()->SetPosDisShape("Sphere");

            // Set angle distribution parameters
            double radius = config.get<double>("sphere_radius");
            single_source->GetPosDist()->SetRadius(radius);
            single_source->GetAngDist()->SetAngDistType("focused");

            if(config.has("sphere_focus_point")) {
                single_source->GetAngDist()->SetFocusPoint(config.get<G4ThreeVector>("sphere_focus_point"));
            } else {
                TRandom3 rand;
                double x = rand.Uniform(source_pos.x() - radius, source_pos.x() + radius); // radius is in mm
                double y = rand.Uniform(source_pos.y() - radius, source_pos.y() + radius); // radius is in mm
                double z = rand.Uniform(source_pos.z() - radius, source_pos.z() + radius); // radius is in mm
                single_source->GetAngDist()->SetFocusPoint(G4ThreeVector(x, y, z));        // argument in mm
            }

        } else if(config.get<std::string>("source_type") == "square") {

            // Set position parameters
            single_source->GetPosDist()->SetPosDisType("Plane");
            single_source->GetPosDist()->SetPosDisShape("Square");
            single_source->GetPosDist()->SetHalfX(config.get<double>("square_side") / 2);
            single_source->GetPosDist()->SetHalfY(config.get<double>("square_side") / 2);

            // Set angle distribution parameters
            single_source->GetAngDist()->SetAngDistType("iso");
            single_source->GetAngDist()->SetMaxTheta(config.get<double>("square_side", ROOT::Math::Pi() / 2));

        } else if(config.get<std::string>("source_type") == "point") {

            // Set position parameters
            single_source->GetPosDist()->SetPosDisType("Point");

            // Set angle distribution parameters
            single_source->GetAngDist()->SetAngDistType("iso");

        } else {

            throw InvalidValueError(config, "source_type", "");
        }

        // Find Geant4 particle
        auto pdg_table = G4ParticleTable::GetParticleTable();
        auto particle_type = config.get<std::string>("particle_type", "");
        std::transform(particle_type.begin(), particle_type.end(), particle_type.begin(), ::tolower);
        auto particle_code = config.get<int>("particle_code", 0);
        G4ParticleDefinition* particle = nullptr;

        if(!particle_type.empty() && particle_code != 0) {
            if(pdg_table->FindParticle(particle_type) == pdg_table->FindParticle(particle_code)) {
                LOG(WARNING) << "particle_type and particle_code given. Continuing because they match.";
                particle = pdg_table->FindParticle(particle_code);
                if(particle == nullptr) {
                    throw InvalidValueError(config, "particle_code", "particle code does not exist.");
                }
            } else {
                throw InvalidValueError(
                    config, "particle_type", "Given particle_type does not match particle_code. Please remove one of them.");
            }
        } else if(particle_type.empty() && particle_code == 0) {
            throw InvalidValueError(config, "particle_code", "Please set particle_code or particle_type.");
        } else if(particle_code != 0) {
            particle = pdg_table->FindParticle(particle_code);
            if(particle == nullptr) {
                throw InvalidValueError(config, "particle_code", "particle code does not exist.");
            }
        } else {
            particle = pdg_table->FindParticle(particle_type);
            if(particle == nullptr) {
                throw InvalidValueError(config, "particle_type", "particle type does not exist.");
            }
        }

        LOG(DEBUG) << "Using particle " << particle->GetParticleName() << " (ID " << particle->GetPDGEncoding() << ").";

        // Set global parameters of the source
        single_source->SetNumberOfParticles(1);
        single_source->SetParticleDefinition(particle);
        // Set the primary track's start time in for the current event to zero:
        single_source->SetParticleTime(0.0);

        // Set energy parameters
        single_source->GetEneDist()->SetEnergyDisType("Gauss");
        single_source->GetEneDist()->SetMonoEnergy(config.get<double>("source_energy"));
        single_source->GetEneDist()->SetBeamSigmaInE(config.get<double>("source_energy_spread", 0.));
    }
}

/**
 * Called automatically for every event
 */
void GeneratorActionG4::GeneratePrimaries(G4Event* event) {
    particle_source_->GeneratePrimaryVertex(event);
}
