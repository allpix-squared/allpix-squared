/**
 * @file
 * @brief Implements the HepMC3 generator file reader module for primary particles
 *
 * @copyright Copyright (c) 2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PrimariesReaderHepMC.hpp"

#include "core/module/exceptions.h"
#include "core/utils/log.h"

#include <G4TransportationManager.hh>

#include <HepMC3/Print.h>
#include <HepMC3/ReaderAscii.h>
#include <HepMC3/ReaderAsciiHepMC2.h>
#include <HepMC3/ReaderRoot.h>

using namespace allpix;

PrimariesReaderHepMC::PrimariesReaderHepMC(const Configuration& config) {

    auto model = config.get<FileModel>("model");
    if(model == FileModel::HEPMC) {
        auto file_path = config.getPathWithExtension("file_name", "txt", true);
        reader_ = std::make_unique<HepMC3::ReaderAscii>(file_path);
    } else if(model == FileModel::HEPMC2) {
        auto file_path = config.getPathWithExtension("file_name", "txt", true);
        reader_ = std::make_unique<HepMC3::ReaderAsciiHepMC2>(file_path);
    } else if(model == FileModel::HEPMCROOT) {
        auto file_path = config.getPathWithExtension("file_name", "root", true);
        reader_ = std::make_unique<HepMC3::ReaderRoot>(file_path);
    }

    if(reader_->failed()) {
        throw InvalidValueError(config, "file_name", "could not open input file");
    }
}

bool PrimariesReaderHepMC::check_vertex_inside_world(const G4ThreeVector& pos) const {
    auto* solid = G4TransportationManager::GetTransportationManager()
                      ->GetNavigatorForTracking()
                      ->GetWorldVolume()
                      ->GetLogicalVolume()
                      ->GetSolid();
    return solid->Inside(pos) == kInside;
}

std::vector<PrimariesReader::Particle> PrimariesReaderHepMC::getParticles() {

    // Read event from input file
    HepMC3::GenEvent evt(HepMC3::Units::MEV, HepMC3::Units::MM);
    reader_->read_event(evt);

    // If we have no more events, end this run
    if(reader_->failed()) {
        throw EndOfRunException("Requesting end of run: end of file reached");
    }

    // Check if this is the requested event, otherwise return an empty vector
    if(static_cast<uint64_t>(evt.event_number()) > eventNum() - 1) {
        LOG(INFO) << "Expecting event " << (eventNum() - 1) << ", found " << static_cast<uint64_t>(evt.event_number())
                  << ", returning empty event";
        return {};
    }

    // FIXME This prints to std::cout. We would need to pass a std::ostream as first parameter
    IFLOG(DEBUG) {
        HepMC3::Print::listing(evt);
        HepMC3::Print::content(evt);
    }

    // Generate particles:
    std::vector<Particle> particles;
    for(const auto& v : evt.vertices()) {

        // Check world boundary of primary vertex
        auto pos = v->position();
        G4ThreeVector vertex_pos(pos.x(), pos.y(), pos.z());
        if(!check_vertex_inside_world(vertex_pos)) {
            LOG(WARNING) << "Vertex at " << vertex_pos << " outside world volume, skipping.";
            continue;
        }

        // Loop over all outgoing particles of this vertex:
        for(const auto& p : v->particles_out()) {

            // Check if this is a final state particle:
            if(p->end_vertex() || p->status() != 1) {
                LOG(DEBUG) << "Skipping particle with ID " << p->pdg_id() << " and status " << p->status()
                           << ", not a final state particle";
                continue;
            }

            // Store particle
            auto momentum = G4ThreeVector(p->momentum().px(), p->momentum().py(), p->momentum().pz());
            particles.emplace_back(p->pdg_id(), p->momentum().e(), momentum, vertex_pos, pos.t());
            LOG(DEBUG) << "Adding particle with ID " << p->pdg_id() << " energy " << p->momentum().e();
        }
    }

    // Return and advance to next tree entry:
    return particles;
}
