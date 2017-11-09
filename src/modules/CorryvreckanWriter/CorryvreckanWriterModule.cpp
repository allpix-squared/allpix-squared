/**
 * @file
 * @brief Implementation of CorryvreckanWriter module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CorryvreckanWriterModule.hpp"

#include <string>
#include <utility>

#include "core/utils/file.h"
#include "core/utils/log.h"

using namespace allpix;

CorryvreckanWriterModule::CorryvreckanWriterModule(Configuration config, Messenger* messenger, GeometryManager* geoManager)
    : Module(std::move(config)), messenger_(messenger), geometryManager_(geoManager) {
    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();
    // Require PixelCharge messages for single detector
    messenger_->bindMulti(this, &CorryvreckanWriterModule::pixel_messages_, MsgFlags::REQUIRED);
    messenger_->addDependency<MCParticleMessage>(this);
    messenger_->addDependency<PixelChargeMessage>(this);
    messenger_->addDependency<PropagatedChargeMessage>(this);
    messenger_->addDependency<DepositedChargeMessage>(this);
}

// Set up the output trees
void CorryvreckanWriterModule::init() {

    LOG(TRACE) << "Initialising module " << getUniqueName();

    // Check if MC data to be saved
    outputMCtruth_ = config_.get<bool>("output_mctruth", false);

    // Create output file and directories
    fileName_ = createOutputFile(
        allpix::add_file_extension(config_.get<std::string>("file_name", "corryvreckanOutput"), "root"), true);
    outputFile_ = std::make_unique<TFile>(fileName_.c_str(), "RECREATE");
    outputFile_->cd();
    outputFile_->mkdir("pixels");
    if(outputMCtruth_) {
        outputFile_->mkdir("mcparticles");
    }

    // Loop over all detectors and make trees for data
    std::vector<std::shared_ptr<Detector>> detectors = geometryManager_->getDetectors();
    for(auto& detector : detectors) {

        // Get the detector ID and type
        std::string detectorID = detector->getName();

        // Create the tree
        std::string objectID = detectorID + "_pixels";
        std::string treeName = detectorID + "_Timepix3_pixels";
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
        std::string treeName_MC = detectorID + "_Timepix3_mcparticles";
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

        std::string detectorID = message->getDetector()->getName();
        std::string objectID = detectorID + "_pixels";
        LOG(DEBUG) << "Receieved " << message->getData().size() << " pixel hits from detector " << detectorID;
        LOG(DEBUG) << "Time on event hits will be " << time_;

        // Loop through all pixels received
        for(auto& allpix_pixel : message->getData()) {

            // Make a new output pixel
            unsigned int pixelX = allpix_pixel.getPixel().getIndex().X();
            unsigned int pixelY = allpix_pixel.getPixel().getIndex().Y();
            double adc = allpix_pixel.getSignal();
            long long int time(time_);
            corryvreckan::Pixel* outputPixel = new corryvreckan::Pixel(detectorID, int(pixelY), int(pixelX), int(adc), time);

            LOG(DEBUG) << "Pixel (" << pixelX << "," << pixelY << ") written to device " << detectorID;

            // Map the pixel to the output tree and write it
            treePixels_[objectID] = outputPixel;
            outputTrees_[objectID]->Fill();

            // If writing MC truth then also write out associated particle info
            if(!outputMCtruth_) {
                continue;
            }

            // Get all associated particles
            auto mcp = allpix_pixel.getMCParticles();
            for(auto& particle : mcp) {

                // Create a new particle object
                std::string objectID_MC = detectorID + "_mcparticles";
                auto mcParticle = new corryvreckan::MCParticle(
                    particle->getParticleID(), particle->getLocalStartPoint(), particle->getLocalEndPoint());

                // Map the mc particle to the output tree and write it
                treeMCParticles_[objectID_MC] = mcParticle;
                outputTreesMC_[objectID_MC]->Fill();
            }
        }
    }

    // Increment the time till the next event
    time_ += 10;
}

// Save the output trees to file
// Set up the output trees
void CorryvreckanWriterModule::finalize() {

    // Loop over all detectors and store the trees
    std::vector<std::shared_ptr<Detector>> detectors = geometryManager_->getDetectors();
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
}
