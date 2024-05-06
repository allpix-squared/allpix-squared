/**
 * @file
 * @brief Implements the particle generator
 * @remark Based on code from John Idarraga
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "GeneratorActionG4.hpp"

#include <limits>
#include <memory>
#include <regex>

#include <G4Event.hh>
#include <G4IonTable.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4RunManager.hh>
#include <G4UImanager.hh>
#include <Math/Vector2D.h>
#include <core/module/exceptions.h>

#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/ROOT.h"
#include "tools/geant4/geant4.h"

using namespace allpix;

/**
 * @brief Parse the given file and save the UI macros in the given vector
 */
static void parse_macro_file_and_prepare_commands(const std::string& file_name, std::vector<std::string>& cmd_list) {
    std::ifstream file(file_name);
    std::string line;

    LOG(TRACE) << "Parsing macro file " << file_name;
    while(std::getline(file, line)) {
        // Check for the "/gps/" pattern in the line:
        if(!line.empty()) {
            if(line.rfind("/gps/number", 0) == 0) {
                throw ModuleError(
                    "The number of particles must be defined in the main configuration file, not in the macro.");
            } else if(line.rfind("/gps/", 0) == 0 || line.at(0) == '#') {
                cmd_list.push_back(line);
            } else {
                LOG(WARNING) << "Ignoring Geant4 macro command: \"" + line + "\" - not related to particle source.";
            }
        }
    }
}

/**
 * @brief Apply GPS UI command from a file. File is only read once and commands are buffered for later calls
 */
static void apply_GPS_UI_commands_from_file(const std::string& file_name) {
    // Commands read from the file and ready to be applied
    static std::vector<std::string> ui_commands;

    // Get the UI commander
    G4UImanager* UI = G4UImanager::GetUIpointer();

    // Parse the macro file only once
    if(ui_commands.empty()) {
        parse_macro_file_and_prepare_commands(file_name, ui_commands);
    }

    // Apply UI macros
    for(auto& cmd : ui_commands) {
        LOG(DEBUG) << "Applying Geant4 macro command: \"" << cmd << "\"";
        UI->ApplyCommand(cmd);
    }
}

// Define radioactive isotopes:
std::map<std::string, std::tuple<int, int, int, double>> GeneratorActionG4::isotopes_ = {
    {"fe55", std::make_tuple(26, 55, 0, 0.)},
    {"am241", std::make_tuple(95, 241, 0, 0.)},
    {"sr90", std::make_tuple(38, 90, 0, 0.)},
    {"co60", std::make_tuple(27, 60, 0, 0.)},
    {"cs137", std::make_tuple(55, 137, 0, 0.)},
};

GeneratorActionG4::GeneratorActionG4(const Configuration& config)
    : particle_source_(std::make_unique<G4GeneralParticleSource>()), config_(config) {

    // Set verbosity of source to off
    particle_source_->SetVerbosity(0);

    // Get source specific parameters
    auto source_type = config_.get<SourceType>("source_type");

    if(source_type == SourceType::MACRO) {
        LOG(INFO) << "Using user macro for particle source.";

        // Get the macro file and apply its commands
        auto file_name = config.getPath("file_name", true);
        apply_GPS_UI_commands_from_file(file_name);

    } else {

        // Get the source and set the centre coordinate of the source
        auto* single_source = particle_source_->GetCurrentSource();
        single_source->GetPosDist()->SetCentreCoords(config_.get<G4ThreeVector>("source_position"));

        // Set position and direction parameters according to shape
        if(source_type == SourceType::BEAM) {

            // Align the -z axis of the system with the direction vector
            auto direction = config_.get<G4ThreeVector>("beam_direction");
            if(fabs(direction.mag() - 1.0) > std::numeric_limits<double>::epsilon()) {
                LOG(WARNING) << "Momentum direction is not a unit vector: magnitude is ignored";
            }
            auto min_element = std::min(std::abs(direction.x()), std::min(std::abs(direction.y()), std::abs(direction.z())));
            G4ThreeVector angref1;
            if(min_element == std::abs(direction.x())) {
                angref1 = direction.cross({1, 0, 0});
            } else if(min_element == std::abs(direction.y())) {
                angref1 = direction.cross({0, 1, 0});
            } else if(min_element == std::abs((direction.z()))) {
                angref1 = direction.cross({0, 0, 1});
            }
            G4ThreeVector angref2 = angref1.cross(direction);

            // Set position parameters
            single_source->GetPosDist()->SetPosDisType("Beam");

            // Get beam_size parameter(s) from config file
            ROOT::Math::XYVector beam_size{};
            try {
                beam_size = config_.get<ROOT::Math::XYVector>("beam_size");
            } catch(InvalidKeyError&) {
                const auto size = config_.get<double>("beam_size", 0);
                beam_size = {size, size};
            }

            auto beam_shape = config_.get<BeamShape>("beam_shape", BeamShape::CIRCLE);
            if(config_.get<bool>("flat_beam", false)) {
                if(beam_shape == BeamShape::RECTANGLE) {
                    single_source->GetPosDist()->SetPosDisType("Plane"); //?
                    single_source->GetPosDist()->SetPosDisShape("Rectangle");
                    single_source->GetPosDist()->SetHalfX((beam_size.x()) / 2);
                    single_source->GetPosDist()->SetHalfY((beam_size.y()) / 2);
                } else if(beam_shape == BeamShape::CIRCLE) {
                    single_source->GetPosDist()->SetPosDisShape("Circle");
                    single_source->GetPosDist()->SetRadius(beam_size.x());
                } else if(beam_shape == BeamShape::ELLIPSE) {
                    single_source->GetPosDist()->SetPosDisShape("Ellipse");
                    single_source->GetPosDist()->SetHalfX((beam_size.x()) / 2);
                    single_source->GetPosDist()->SetHalfY((beam_size.y()) / 2);
                } else {
                    throw InvalidValueError(config_, "beam_shape", "Cannot use this beam shape with flat beams");
                }
            } else {
                if(beam_shape == BeamShape::CIRCLE) {
                    single_source->GetPosDist()->SetBeamSigmaInR(beam_size.x());
                } else if(beam_shape == BeamShape::ELLIPSE || beam_shape == BeamShape::RECTANGLE) {
                    single_source->GetPosDist()->SetBeamSigmaInX((beam_size.x()) / 2);
                    single_source->GetPosDist()->SetBeamSigmaInY((beam_size.y()) / 2);
                } else {
                    throw InvalidValueError(config_, "beam_shape", "This beam shape can only be used with flat beams");
                }
            }

            single_source->GetPosDist()->SetPosRot1(angref1);
            single_source->GetPosDist()->SetPosRot2(angref2);

            // Set angle distribution parameters
            // NOTE beam2d will always fire in the -z direction of the system
            single_source->GetAngDist()->SetAngDistType("beam2d");
            single_source->GetAngDist()->DefineAngRefAxes("angref1", angref1);
            single_source->GetAngDist()->DefineAngRefAxes("angref2", angref2);
            auto divergence = config_.get<G4TwoVector>("beam_divergence", G4TwoVector(0., 0.));
            single_source->GetAngDist()->SetBeamSigmaInAngX(divergence.x());
            single_source->GetAngDist()->SetBeamSigmaInAngY(divergence.y());

        } else if(source_type == SourceType::SPHERE) {

            // Set position parameters
            single_source->GetPosDist()->SetPosDisType("Surface");
            single_source->GetPosDist()->SetPosDisShape("Sphere");

            // Set angle distribution parameters
            single_source->GetPosDist()->SetRadius(config_.get<double>("sphere_radius"));

            if(config_.has("sphere_focus_point")) {
                single_source->GetAngDist()->SetAngDistType("focused");
                single_source->GetAngDist()->SetFocusPoint(config_.get<G4ThreeVector>("sphere_focus_point"));
            } else {
                single_source->GetAngDist()->SetAngDistType("cos");
            }

        } else if(source_type == SourceType::SQUARE) {

            // Set position parameters
            single_source->GetPosDist()->SetPosDisType("Plane");
            single_source->GetPosDist()->SetPosDisShape("Square");
            single_source->GetPosDist()->SetHalfX(config_.get<double>("square_side") / 2);
            single_source->GetPosDist()->SetHalfY(config_.get<double>("square_side") / 2);

            // Set angle distribution parameters
            single_source->GetAngDist()->SetAngDistType("iso");
            single_source->GetAngDist()->SetMaxTheta(config_.get<double>("square_angle", ROOT::Math::Pi()) / 2);

        } else if(source_type == SourceType::POINT) {

            // Set position parameters
            single_source->GetPosDist()->SetPosDisType("Point");

            // Set angle distribution parameters
            single_source->GetAngDist()->SetAngDistType("iso");
        }

        // Find Geant4 particle
        auto* pdg_table = G4ParticleTable::GetParticleTable();
        particle_type_ = allpix::transform(config_.get<std::string>("particle_type", ""), ::tolower);
        auto particle_code = config_.get<int>("particle_code", 0);
        G4ParticleDefinition* particle = nullptr;

        if(!particle_type_.empty() && particle_code != 0) {
            if(pdg_table->FindParticle(particle_type_) == pdg_table->FindParticle(particle_code)) {
                LOG(WARNING) << "particle_type and particle_code given. Continuing because they match.";
                particle = pdg_table->FindParticle(particle_code);
                if(particle == nullptr) {
                    throw InvalidValueError(config_, "particle_code", "particle code does not exist.");
                }
            } else {
                throw InvalidValueError(config_,
                                        "particle_type",
                                        "Given particle_type does not match particle_code. Please remove one of them.");
            }
        } else if(particle_type_.empty() && particle_code == 0) {
            throw InvalidValueError(config_, "particle_code", "Please set particle_code or particle_type.");
        } else if(particle_code != 0) {
            particle = pdg_table->FindParticle(particle_code);
            if(particle == nullptr) {
                throw InvalidValueError(config_, "particle_code", "particle code does not exist.");
            }
        } else if(isotopes_.find(particle_type_) != isotopes_.end() || particle_type_.substr(0, 3) == "ion") {
            // In the case we are using a multithreaded version of Geant4, Ion tables may not be ready to use now
            // so we do use them later
            initialize_ion_as_particle_ = true;
        } else {
            particle = pdg_table->FindParticle(particle_type_);
            if(particle == nullptr) {
                throw InvalidValueError(config_, "particle_type", "particle type does not exist.");
            }
        }

        if(particle != nullptr) {
            LOG(DEBUG) << "Using particle " << particle->GetParticleName() << " (ID " << particle->GetPDGEncoding() << ").";
            // Set global parameters of the source
            single_source->SetNumberOfParticles(1);
            single_source->SetParticleDefinition(particle);
            // Set the primary track's start time in for the current event to zero:
            single_source->SetParticleTime(0.0);
        }

        // Set energy parameters
        single_source->GetEneDist()->SetEnergyDisType("Gauss");
        single_source->GetEneDist()->SetMonoEnergy(config_.get<double>("source_energy"));
        single_source->GetEneDist()->SetBeamSigmaInE(config_.get<double>("source_energy_spread", 0.));
    }
}

/**
 * Called automatically for every event
 */
void GeneratorActionG4::GeneratePrimaries(G4Event* event) {

    // G4IonTable is only ready after initialization, so we need to pick the particle here and assign it to the source:
    if(initialize_ion_as_particle_) {
        auto* single_source = particle_source_->GetCurrentSource();
        G4ParticleDefinition* particle = nullptr;

        if(isotopes_.find(particle_type_) != isotopes_.end()) {
            auto isotope = isotopes_[particle_type_];
            // Set radioactive isotope:
            particle = G4IonTable::GetIonTable()->GetIon(std::get<0>(isotope), std::get<1>(isotope), std::get<3>(isotope));

            // Force the radioactive isotope to decay immediately:
            particle->SetPDGLifeTime(0.);

            single_source->SetParticleCharge(std::get<2>(isotope));

            // Warn about non-zero source energy:
            if(config_.get<double>("source_energy") > 0) {
                LOG_ONCE(WARNING)
                    << "A radioactive isotope is used as particle source, but the source energy is not set to zero.";
            }
        } else if(particle_type_.substr(0, 3) == "ion") {
            // Parse particle type as ion with components /Z/A/Q/E/D
            std::smatch ion;
            if(std::regex_match(particle_type_,
                                ion,
                                std::regex("ion/([0-9]+)/([0-9]+)/([-+]?[0-9]+)/([0-9.]+(?:[a-zA-Z]+)?)/(true|false)")) &&
               ion.ready()) {
                particle = G4IonTable::GetIonTable()->GetIon(
                    allpix::from_string<int>(ion[1]), allpix::from_string<int>(ion[2]), allpix::from_string<double>(ion[4]));
                if(allpix::from_string<bool>(ion[5])) {
                    particle->SetPDGLifeTime(0.);
                }
                single_source->SetParticleCharge(allpix::from_string<int>(ion[3]));
            } else if(std::regex_match(
                          particle_type_, ion, std::regex("ion/([0-9]+)/([0-9]+)/([-+]?[0-9]+)/([0-9.]+(?:[a-zA-Z]+)?)")) &&
                      ion.ready()) {
                // Parse old declaration with /Z/A/Q/E
                particle = G4IonTable::GetIonTable()->GetIon(
                    allpix::from_string<int>(ion[1]), allpix::from_string<int>(ion[2]), allpix::from_string<double>(ion[4]));
                single_source->SetParticleCharge(allpix::from_string<int>(ion[3]));
                LOG(WARNING) << "Using \"ion/Z/A/Q/E\" is deprecated and superseded by \"ion/Z/A/Q/E/D\".";
            } else {
                throw InvalidValueError(config_, "particle_type", "cannot parse parameters for ion.");
            }
        }

        if(particle != nullptr) {
            // Set global parameters of the source
            single_source->SetNumberOfParticles(1);
            single_source->SetParticleDefinition(particle);
            // Set the primary track's start time in for the current event to zero:
            single_source->SetParticleTime(0.0);

            // mark the initialization done
            initialize_ion_as_particle_ = false;

            LOG(DEBUG) << "Using ion " << particle->GetParticleName() << " (ID " << particle->GetPDGEncoding() << ") with "
                       << Units::display(particle->GetPDGLifeTime(), {"s", "ns"}) << " lifetime.";
        } else {
            throw InvalidValueError(config_, "particle_type", "failed to fetch or create ion.");
        }
    }

    particle_source_->GeneratePrimaryVertex(event);
}

GeneratorActionInitializationMaster::GeneratorActionInitializationMaster(const Configuration& config)
    : particle_source_(std::make_unique<G4GeneralParticleSource>()) {

    // Set verbosity of source to off
    particle_source_->SetVerbosity(0);

    // Get source specific parameters
    auto source_type = config.get<GeneratorActionG4::SourceType>("source_type");

    if(source_type == GeneratorActionG4::SourceType::MACRO) {
        LOG(INFO) << "Using user macro for particle source.";

        // Get the macro file and apply its commands
        auto file_name = config.getPath("file_name", true);
        apply_GPS_UI_commands_from_file(file_name);
    }
}
