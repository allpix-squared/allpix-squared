/**
 * @file
 * @brief Implements the HepMC3 generator file reader module for primary particles
 *
 * @copyright Copyright (c) 2022-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PrimariesReaderHepMC.hpp"

#include "core/module/exceptions.h"
#include "core/utils/log.h"

#include <HepMC3/Print.h>
#include <HepMC3/ReaderAscii.h>
#include <HepMC3/ReaderAsciiHepMC2.h>
#include <HepMC3/ReaderRoot.h>
#include <HepMC3/ReaderRootTree.h>

using namespace allpix;

PrimariesReaderHepMC::PrimariesReaderHepMC(const Configuration& config) {

    auto model = config.get<FileModel>("model");
    std::filesystem::path file_path;
    if(model == FileModel::HEPMC) {
        file_path = config.getPathWithExtension("file_name", "txt", true);
        reader_ = std::make_unique<HepMC3::ReaderAscii>(file_path);
    } else if(model == FileModel::HEPMC2) {
        file_path = config.getPathWithExtension("file_name", "txt", true);
        reader_ = std::make_unique<HepMC3::ReaderAsciiHepMC2>(file_path);
    } else if(model == FileModel::HEPMCROOT) {
        file_path = config.getPathWithExtension("file_name", "root", true);
        reader_ = std::make_unique<HepMC3::ReaderRoot>(file_path);
    } else if(model == FileModel::HEPMCTTREE) {
        file_path = config.getPathWithExtension("file_name", "root", true);
        reader_ = std::make_unique<HepMC3::ReaderRootTree>(file_path);
    } else {
        throw InvalidValueError(config, "model", "failed to instantiate file reader");
    }

    if(reader_->failed()) {
        throw InvalidValueError(config, "file_name", "could not open input file");
    }
    LOG(INFO) << "Successfully opened data file " << file_path;
}

std::vector<PrimariesReader::Particle> PrimariesReaderHepMC::getParticles() {

    // Read event from input file
    HepMC3::GenEvent evt(HepMC3::Units::MEV, HepMC3::Units::MM);
    LOG(DEBUG) << "Reading event " << eventNum() << " from HepMC3 file";
    reader_->read_event(evt);

    // If we have no more events, end this run
    if(reader_->failed()) {
        throw EndOfRunException("Requesting end of run: end of file reached");
    }

    // Check if this is the requested event, otherwise act!
    while(static_cast<uint64_t>(evt.event_number()) < eventNum() - 1) {
        LOG(INFO) << "HepMC3 event " << evt.event_number() << " too early, dropping.";
        reader_->read_event(evt);
    }

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

        auto pos = v->position();

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
            particles.emplace_back(
                p->pdg_id(), p->momentum().e(), momentum, G4ThreeVector(pos.x(), pos.y(), pos.z()), pos.t());
            LOG(DEBUG) << "Adding particle with ID " << p->pdg_id() << " energy " << p->momentum().e();
        }
    }

    // Return and advance to next tree entry:
    return particles;
}
