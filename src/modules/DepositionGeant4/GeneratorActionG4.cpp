/**
 * @file
 * @brief Implements the particle generator
 * @remark Based on code from John Idarraga
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied
 * verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities
 * granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "GeneratorActionG4.hpp"

#include <limits>
#include <memory>

#include <G4Event.hh>
#include <G4GeneralParticleSource.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <Randomize.hh>
#include <array>

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
    auto source_type = config.get<std::string>("beam_source_type", "");

    // Find Geant4 particle
    auto pdg_table = G4ParticleTable::GetParticleTable();
    auto particle_type = config.get<std::string>("particle_type", "");
    std::transform(particle_type.begin(), particle_type.end(), particle_type.begin(), ::tolower);
    auto particle_code = config.get<int>("particle_code", 0);
    G4ParticleDefinition* particle = nullptr;

    if(source_type.empty() && !particle_type.empty() && particle_code != 0) {
        // if(!particle_type.empty() && particle_code != 0) {
        if(pdg_table->FindParticle(particle_type) == pdg_table->FindParticle(particle_code)) {
            LOG(WARNING) << "particle_type and particle_code given. Continuing because t    hey match.";
            particle = pdg_table->FindParticle(particle_code);
            if(particle == nullptr) {
                throw InvalidValueError(config, "particle_code", "particle code does not exist.");
            }
        } else {
            throw InvalidValueError(
                config, "particle_type", "Given particle_type does not match particle_code. Please remove one of them.");
        }
    } else if(source_type.empty() && particle_type.empty() && particle_code == 0) {
        throw InvalidValueError(config, "particle_code", "Please set particle_code or particle_type.");
    } else if(source_type.empty() && particle_code != 0) {
        particle = pdg_table->FindParticle(particle_code);
        if(particle == nullptr) {
            throw InvalidValueError(config, "particle_code", "particle code does not exist.");
        }
    } else if(source_type.empty() && !particle_type.empty()) {
        particle = pdg_table->FindParticle(particle_type);
        if(particle == nullptr) {
            throw InvalidValueError(config, "particle_type", "particle type does not exist.");
        }
    } else {
        if(source_type.empty()) {
            throw InvalidValueError(config, "source_type", "Please set source type.");
        }
    }

    LOG(DEBUG) << "Using particle " << particle->GetParticleName() << " (ID " << particle->GetPDGEncoding() << ").";

    // Set global parameters of the source
    if(!particle_type.empty() || particle_code != 0) {
        single_source->SetNumberOfParticles(1);
        single_source->SetParticleDefinition(particle);
        // Set the primary track's start time in for the current event to zero:
        single_source->SetParticleTime(0.0);
    }

    // Set position parameters
    single_source->GetPosDist()->SetPosDisType("Beam");
    single_source->GetPosDist()->SetBeamSigmaInR(config.get<double>("beam_size", 0));
    single_source->GetPosDist()->SetCentreCoords(config.get<G4ThreeVector>("beam_position"));

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
    if(!particle_type.empty() || particle_code != 0) {
        single_source->GetAngDist()->SetParticleMomentumDirection(direction);
    }

    // Set energy parameters
    auto energy_type = config.get<std::string>("beam_energy_type", "");
    if(energy_type == "User") {
        single_source->GetEneDist()->SetEnergyDisType(energy_type);
        auto beam_hist_point = config.getArray<G4ThreeVector>("beam_hist_point");
        for(auto& hist_point : beam_hist_point) {
            single_source->GetEneDist()->UserEnergyHisto(hist_point);
        }
    } else if(source_type == "Iron-55") {
        std::stringstream ss;
        ss << "gamma";
        particle_type.assign(ss.str());
        std::transform(particle_type.begin(), particle_type.end(), particle_type.begin(), ::tolower);
        particle = pdg_table->FindParticle(particle_type);
        single_source->SetNumberOfParticles(1);
        single_source->SetParticleDefinition(particle);
        single_source->SetParticleTime(0.0);
        single_source->GetAngDist()->SetParticleMomentumDirection(direction);
        single_source->GetEneDist()->SetEnergyDisType("User");
        G4double energy_hist_1[10];
        G4double intensity_hist_1[10];
        for(G4int i = 0; i < 10; i++) {
            energy_hist_1[i] = 0.0059 + 0.0001 * (1 - 2 * G4UniformRand());
            intensity_hist_1[i] = 28. + 0.1 * (1 - 2 * G4UniformRand());
            single_source->GetEneDist()->UserEnergyHisto(G4ThreeVector(energy_hist_1[i], intensity_hist_1[i], 0.));
        }
        G4double energy_hist_2[10];
        G4double intensity_hist_2[10];
        for(G4int i = 0; i < 10; i++) {
            energy_hist_2[i] = 0.00649 + 0.0001 * (1 - 2 * G4UniformRand());
            intensity_hist_2[i] = 2.85 + 0.01 * (1 - 2 * G4UniformRand());
            single_source->GetEneDist()->UserEnergyHisto(G4ThreeVector(energy_hist_2[i], intensity_hist_2[i], 0.));
        }
    } else {
        single_source->GetEneDist()->SetEnergyDisType("Gauss");
        single_source->GetEneDist()->SetMonoEnergy(config.get<double>("beam_energy"));
    }
    single_source->GetEneDist()->SetBeamSigmaInE(config.get<double>("beam_energy_spread", 0.));
}

/**
 * Called automatically for every event
 */
void GeneratorActionG4::GeneratePrimaries(G4Event* event) {
    particle_source_->GeneratePrimaryVertex(event);
}
