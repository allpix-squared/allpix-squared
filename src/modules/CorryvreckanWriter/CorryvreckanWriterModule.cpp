/**
 * @file
 * @brief Implementation of CorryvreckanWriter module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "CorryvreckanWriterModule.hpp"

#include <Math/RotationZYX.h>
#include <TProcessID.h>

#include <fstream>
#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

CorryvreckanWriterModule::CorryvreckanWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geoManager)
    : SequentialModule(config), messenger_(messenger), geometryManager_(geoManager) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Require PixelHit messages for single detector
    messenger_->bindMulti<PixelHitMessage>(this, MsgFlags::REQUIRED);

    config_.setDefault("file_name", "corryvreckanOutput.root");
    config_.setDefault("geometry_file", "corryvreckanGeometry.conf");
    config_.setDefault("global_timing", false);
    config_.setDefault("output_mctruth", true);
}

// Set up the output trees
void CorryvreckanWriterModule::initialize() {

    // Check if MC data to be saved
    output_mc_truth_ = config_.get<bool>("output_mctruth");

    reference_ = config_.get<std::string>("reference");
    if(!geometryManager_->hasDetector(reference_)) {
        throw InvalidValueError(config_, "reference", "detector not defined");
    }
    dut_ = config_.getArray<std::string>("dut", std::vector<std::string>());
    for(auto& dut : dut_) {
        if(!geometryManager_->hasDetector(dut)) {
            throw InvalidValueError(config_, "dut", "detector not defined");
        }
    }

    // Select pixel hit timing information to be saved:
    timing_global_ = config_.get<bool>("global_timing");

    // Create output file and directories
    fileName_ = createOutputFile(config_.get<std::string>("file_name"), "root");
    LOG(TRACE) << "Creating output file \"" << fileName_ << "\"";
    output_file_ = std::make_unique<TFile>(fileName_.c_str(), "RECREATE");
    output_file_->cd();

    // Create geometry file:
    geometryFileName_ = createOutputFile(config_.get<std::string>("geometry_file"), "conf");

    // Create trees:
    LOG(TRACE) << "Booking event tree";
    event_tree_ = std::make_unique<TTree>("Event", (std::string("Tree of Events").c_str()));
    event_tree_->Bronch("global", "corryvreckan::Event", &event_);

    LOG(TRACE) << "Booking pixel tree";
    pixel_tree_ = std::make_unique<TTree>("Pixel", (std::string("Tree of Pixels").c_str()));

    if(output_mc_truth_) {
        LOG(TRACE) << "Booking MCParticle tree";
        mcparticle_tree_ = std::make_unique<TTree>("MCParticle", (std::string("Tree of MCParticles").c_str()));
    }

    // Initialise the time
    time_ = 0;
}

// Make instantiations of Corryvreckan pixels, and store these in the trees during run time
void CorryvreckanWriterModule::run(Event* event) {
    auto root_lock = root_process_lock();

    auto pixel_messages = messenger_->fetchMultiMessage<PixelHitMessage>(this, event);

    // Retrieve current object count:
    auto object_count = TProcessID::GetObjectCount();

    LOG(TRACE) << "Processing event " << event->number;

    // Create and store a new Event:
    event_ = new corryvreckan::Event(time_, time_ + 5);
    LOG(DEBUG) << "Defining event for Corryvreckan: [" << Units::display(event_->start(), {"ns", "um"}) << ","
               << Units::display(event_->end(), {"ns", "um"}) << "]";
    event_tree_->Fill();

    // Events start with 1, pre-filling only with empty events before:
    auto event_id = event->number - 1;

    // Loop through all received messages
    for(auto& message : pixel_messages) {

        auto detector_name = message->getDetector()->getName();

        // Calculate coordinate system offset for MC truth storage
        // Corryvreckan coordinates originate at the center of the matrix
        auto detector_model = message->getDetector()->getModel();
        auto pixel_size = detector_model->getPixelSize();
        ROOT::Math::XYZVector offset(pixel_size.X() / 2, pixel_size.Y() / 2, 0);
        offset -= detector_model->getMatrixSize() / 2;

        LOG(DEBUG) << "Received " << message->getData().size() << " pixel hits from detector " << detector_name;

        if(write_list_px_.find(detector_name) == write_list_px_.end()) {
            write_list_px_[detector_name] = new std::vector<corryvreckan::Pixel*>();
            pixel_tree_->Bronch(detector_name.c_str(),
                                std::string("std::vector<corryvreckan::Pixel*>").c_str(),
                                &write_list_px_[detector_name]);

            if(event_id > 0) {
                LOG(DEBUG) << "Pre-filling new branch " << detector_name << " of corryvreckan::Pixel with " << event_id
                           << " empty events";
                auto* branch = pixel_tree_->GetBranch(detector_name.c_str());
                for(unsigned int i = 0; i < event_id; ++i) {
                    branch->Fill();
                }
            }
        }

        if(output_mc_truth_ && write_list_mcp_.find(detector_name) == write_list_mcp_.end()) {
            write_list_mcp_[detector_name] = new std::vector<corryvreckan::MCParticle*>();
            mcparticle_tree_->Bronch(detector_name.c_str(),
                                     std::string("std::vector<corryvreckan::MCParticle*>").c_str(),
                                     &write_list_mcp_[detector_name]);

            if(event_id > 0) {

                LOG(DEBUG) << "Pre-filling new branch " << detector_name << " of corryvreckan::MCParticle with " << event_id
                           << " empty events";
                auto* branch = mcparticle_tree_->GetBranch(detector_name.c_str());
                for(unsigned int i = 0; i < event_id; ++i) {
                    branch->Fill();
                }
            }
        }

        // Fill the branch vector
        for(const auto& apx_pixel : message->getData()) {
            auto* corry_pixel = new corryvreckan::Pixel(
                detector_name,
                apx_pixel.getPixel().getIndex().X(),
                apx_pixel.getPixel().getIndex().Y(),
                static_cast<int>(apx_pixel.getSignal()),
                apx_pixel.getSignal(),
                (timing_global_ ? event_->start() + apx_pixel.getGlobalTime() : apx_pixel.getLocalTime()));
            write_list_px_[detector_name]->push_back(corry_pixel);

            // If writing MC truth then also write out associated particle info
            if(!output_mc_truth_) {
                continue;
            }

            // Get all associated particles
            auto mcp = apx_pixel.getMCParticles();
            LOG(DEBUG) << "Received " << mcp.size() << " Monte Carlo particles from pixel hit";
            for(auto& particle : mcp) {
                auto* mcParticle = new corryvreckan::MCParticle(
                    detector_name,
                    particle->getParticleID(),
                    particle->getLocalStartPoint() + offset,
                    particle->getLocalEndPoint() + offset,
                    (timing_global_ ? event_->start() + particle->getGlobalTime() : particle->getLocalTime()));
                write_list_mcp_[detector_name]->push_back(mcParticle);
            }
        }
    }

    LOG(TRACE) << "Writing new objects to tree";
    output_file_->cd();

    pixel_tree_->Fill();
    if(output_mc_truth_) {
        mcparticle_tree_->Fill();
    }

    // Clear the current write lists
    for(auto& index_data : write_list_px_) {
        for(auto& pixel : (*index_data.second)) {
            delete pixel;
        }
        index_data.second->clear();
    }

    // Clear the current write lists
    for(auto& index_data : write_list_mcp_) {
        for(auto& mcp : (*index_data.second)) {
            delete mcp;
        }
        index_data.second->clear();
    }

    // Increment the time till the next event
    time_ += 10;

    // Delete the currently stored event object
    delete event_;

    // Reset object count:
    TProcessID::SetObjectCount(object_count);
}

// Save the output trees to file
void CorryvreckanWriterModule::finalize() {

    // Finish writing to output file
    output_file_->Write();

    // Print statistics
    LOG(STATUS) << "Wrote output data to file:" << std::endl << fileName_;

    // Loop over all detectors and store the geometry:
    // Write geometry:
    std::ofstream geometry_file;
    if(!geometryFileName_.empty()) {
        geometry_file.open(geometryFileName_, std::ios_base::out | std::ios_base::trunc);
        if(!geometry_file.good()) {
            throw ModuleError("Cannot write to Corryvreckan geometry file");
        }

        geometry_file << "# Allpix Squared detector geometry - https://cern.ch/allpix-squared" << std::endl << std::endl;

        auto detectors = geometryManager_->getDetectors();
        for(auto& detector : detectors) {
            geometry_file << "[" << detector->getName() << "]" << std::endl;
            geometry_file << "position = " << Units::display(detector->getPosition().x(), {"mm", "um"}) << ", "
                          << Units::display(detector->getPosition().y(), {"mm", "um"}) << ", "
                          << Units::display(detector->getPosition().z(), {"mm", "um"}) << std::endl;

            // Transform the rotation matrix to a ZYX rotation and invert it to get a XYZ rotation
            // This way we stay compatible to old Corryvreckan versions which only support XYZ.
            geometry_file << "orientation_mode = \"xyz\"" << std::endl;
            ROOT::Math::RotationZYX rotations(detector->getOrientation().Inverse());
            geometry_file << "orientation = " << Units::display(-rotations.Psi(), "deg") << ", "
                          << Units::display(-rotations.Theta(), "deg") << ", " << Units::display(-rotations.Phi(), "deg")
                          << std::endl;

            auto model = detector->getModel();
            geometry_file << "type = \"" << model->getType() << "\"" << std::endl;
            geometry_file << "pixel_pitch = " << Units::display(model->getPixelSize().x(), "um") << ", "
                          << Units::display(model->getPixelSize().y(), "um") << std::endl;
            geometry_file << "number_of_pixels = " << model->getNPixels().x() << ", " << model->getNPixels().y()
                          << std::endl;
            // Time resolution hard-coded as 5ns due to time structure of written out events: events of length 5ns, with a
            // gap of 10ns in between events
            geometry_file << "time_resolution = 5ns" << std::endl;

            std::string roles;
            if(detector->getName() == reference_) {
                roles += "reference";
            }
            if(std::find(dut_.begin(), dut_.end(), detector->getName()) != dut_.end()) {
                if(!roles.empty()) {
                    roles += ",";
                }
                roles += "dut";
            }
            if(!roles.empty()) {
                geometry_file << "role = " << roles << std::endl;
            }

            // Get the material budget if available:
            auto budget = geometryManager_->getExternalObject<double>(detector->getName(), "material_budget");
            if(budget != nullptr) {
                LOG(DEBUG) << "Found calculated material budget for detector " << detector->getName() << ", storing.";
                geometry_file << "material_budget = " << *budget << std::endl;
            }
            geometry_file << std::endl;
        }
    }
}
