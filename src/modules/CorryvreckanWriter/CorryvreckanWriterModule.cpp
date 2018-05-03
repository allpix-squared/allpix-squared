/**
 * @file
 * @brief Implementation of CorryvreckanWriter module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CorryvreckanWriterModule.hpp"

#include <Math/RotationZYX.h>

#include <fstream>
#include <string>
#include <utility>

#include "core/utils/file.h"
#include "core/utils/log.h"

using namespace allpix;

CorryvreckanWriterModule::CorryvreckanWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geoManager)
    : Module(config), messenger_(messenger), geometryManager_(geoManager) {

    // Require PixelCharge messages for single detector
    messenger_->bindMulti(this, &CorryvreckanWriterModule::pixel_messages_, MsgFlags::REQUIRED);

    config_.setDefault("file_name", "corryvreckanOutput.root");
    config_.setDefault("geometry_file", "corryvreckanGeometry.conf");
    config_.setDefault("output_mctruth", false);
}

// Set up the output trees
void CorryvreckanWriterModule::init() {

    // Check if MC data to be saved
    outputMCtruth_ = config_.get<bool>("output_mctruth", false);

    // Create output file and directories
    fileName_ = createOutputFile(allpix::add_file_extension(config_.get<std::string>("file_name"), "root"));
    outputFile_ = std::make_unique<TFile>(fileName_.c_str(), "RECREATE");
    outputFile_->cd();
    outputFile_->mkdir("pixels");
    if(outputMCtruth_) {
        outputFile_->mkdir("mcparticles");
    }

    // Create geometry file:
    geometryFileName_ = createOutputFile(allpix::add_file_extension(config_.get<std::string>("geometry_file"), "conf"));

    // Loop over all detectors and make trees for data
    auto detectors = geometryManager_->getDetectors();
    for(auto& detector : detectors) {

        // Get the detector ID and type
        std::string detectorID = detector->getName();
        std::string detectorModel = detector->getModel()->getType();

        // Create the tree
        std::string objectID = detectorID + "_pixels";
        std::string treeName = detectorID + "_" + detectorModel + "_pixels"; // NOLINT
        outputTrees_[objectID] = new TTree(treeName.c_str(), treeName.c_str());
        outputTrees_[objectID]->Branch("time", &time_);

        // Map the pixel object to the tree
        treePixels_[objectID] = new corryvreckan::Pixel();
        outputTrees_[objectID]->Branch("pixels", &treePixels_[objectID]);

        // If MC truth needed then make trees for output
        if(!outputMCtruth_) {
            continue;
        }

        // Create the tree
        std::string objectID_MC = detectorID + "_mcparticles";
        std::string treeName_MC = detectorID + "_" + detectorModel + "_mcparticles"; // NOLINT
        outputTreesMC_[objectID_MC] = new TTree(treeName_MC.c_str(), treeName_MC.c_str());
        outputTreesMC_[objectID_MC]->Branch("time", &time_);

        // Map the mc particle object to the tree
        treeMCParticles_[objectID_MC] = new corryvreckan::MCParticle();
        outputTreesMC_[objectID_MC]->Branch("mcparticles", &treeMCParticles_[objectID_MC]);
    }

    // Initialise the time
    time_ = 0;
}

// Make instantiations of Corryvreckan pixels, and store these in the trees during run time
void CorryvreckanWriterModule::run(unsigned int) {

    // Loop through all receieved messages
    for(auto& message : pixel_messages_) {

        auto detectorID = message->getDetector()->getName();
        auto objectID = detectorID + "_pixels";
        LOG(DEBUG) << "Receieved " << message->getData().size() << " pixel hits from detector " << detectorID;
        LOG(DEBUG) << "Time on event hits will be " << time_;

        // Loop through all pixels received
        for(auto& allpix_pixel : message->getData()) {

            // Make a new output pixel
            unsigned int pixelX = allpix_pixel.getPixel().getIndex().X();
            unsigned int pixelY = allpix_pixel.getPixel().getIndex().Y();
            double adc = allpix_pixel.getSignal();
            double time(time_);
            auto outputPixel = new corryvreckan::Pixel(detectorID, int(pixelY), int(pixelX), int(adc), time);

            LOG(DEBUG) << "Pixel (" << pixelX << "," << pixelY << ") written to device " << detectorID;

            // Map the pixel to the output tree and write it
            treePixels_[objectID] = outputPixel;
            outputTrees_[objectID]->Fill();
            delete outputPixel;

            // If writing MC truth then also write out associated particle info
            if(!outputMCtruth_) {
                continue;
            }

            // Get all associated particles
            auto mcp = allpix_pixel.getMCParticles();
            for(auto& particle : mcp) {

                // Create a new particle object
                std::string objectID_MC = detectorID + "_mcparticles";
                auto mcParticle = new corryvreckan::MCParticle(detectorID,
                                                               particle->getParticleID(),
                                                               particle->getLocalStartPoint(),
                                                               particle->getLocalEndPoint(),
                                                               time);

                // Map the mc particle to the output tree and write it
                treeMCParticles_[objectID_MC] = mcParticle;
                LOG(DEBUG) << "MC particle started locally at (" << mcParticle->getLocalStart().X() << ","
                           << mcParticle->getLocalStart().Y() << ") and ended at " << mcParticle->getLocalEnd().X() << ","
                           << mcParticle->getLocalEnd().Y() << ")";
                LOG(DEBUG) << "MC particle started globally at (" << particle->getGlobalStartPoint().X() << ","
                           << particle->getGlobalStartPoint().Y() << ") and ended at " << particle->getGlobalEndPoint().X()
                           << "," << particle->getGlobalEndPoint().Y() << ")";
                outputTreesMC_[objectID_MC]->Fill();
                delete mcParticle;
            }
        }
    }

    // Increment the time till the next event
    time_ += 10;
}

// Save the output trees to file
void CorryvreckanWriterModule::finalize() {

    // Loop over all detectors and store the trees
    auto detectors = geometryManager_->getDetectors();
    for(auto& detector : detectors) {

        // Get the detector ID and type
        std::string detectorID = detector->getName();
        std::string objectID = detectorID + "_pixels";

        // Move to the write output file
        outputFile_->cd();
        outputFile_->cd("pixels");
        outputTrees_[objectID]->Write();

        // Clean up the tree and remove object pointer
        delete outputTrees_[objectID];
        treePixels_[objectID] = nullptr;

        // Write the MC truth
        if(!outputMCtruth_) {
            continue;
        }

        std::string objectID_MC = detectorID + "_mcparticles";
        outputFile_->cd("mcparticles");
        outputTreesMC_[objectID_MC]->Write();

        // Clean up the tree and remove object pointer
        delete outputTreesMC_[objectID_MC];
        treeMCParticles_[objectID_MC] = nullptr;
    }
    outputFile_->Close();

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

            geometry_file << std::endl;
        }
    }
}
