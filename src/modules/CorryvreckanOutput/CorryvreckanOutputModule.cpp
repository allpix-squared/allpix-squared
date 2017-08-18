/**
 * @file
 * @brief Implementation of [CorryvreckanOutput] module
 * @copyright MIT License
 */

#include "CorryvreckanOutputModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

CorryvreckanOutputModule::CorryvreckanOutputModule(Configuration config, Messenger* messenger, GeometryManager* geoManager)
    : Module(config), messenger_(messenger), config_(std::move(config)), geometryManager_(geoManager) {
    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();
    // Require PixelCharge messages for single detector
    messenger_->bindMulti(this, &CorryvreckanOutputModule::pixel_messages_, MsgFlags::REQUIRED);
}

// Set up the output trees
void CorryvreckanOutputModule::init() {

    LOG(TRACE) << "Initialising module " << getUniqueName();

    // Create output file and directories
    fileName_ = getOutputPath(config_.get<std::string>("file_name", "corryvreckanOutput") + ".root", true);
    outputFile_ = std::make_unique<TFile>(fileName_.c_str(), "RECREATE");
    outputFile_->cd();
    outputFile_->mkdir("pixels");

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
        treePixels_[objectID] = new Corryvreckan::Pixel();
        outputTrees_[objectID]->Branch("pixels", &treePixels_[objectID]);
    }

    // Initialise the time
    time_ = 0;
}

// Make instantiations of Corryvreckan pixels, and store these in the trees during run time
void CorryvreckanOutputModule::run(unsigned int) {

    LOG(TRACE) << "Running module " << getUniqueName();

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
            unsigned int adc = allpix_pixel.getPixel().getIndex().Y();
            Corryvreckan::Pixel* outputPixel = new Corryvreckan::Pixel(detectorID, int(pixelX), int(pixelY), int(adc));

            // Map the pixel to the output tree and write it
            treePixels_[objectID] = outputPixel;
            outputTrees_[objectID]->Fill();
        }
    }

    // Increment the time till the next event
    time_ += 1;
}

// Save the output trees to file
// Set up the output trees
void CorryvreckanOutputModule::finalize() {

    LOG(TRACE) << "Finalising module " << getUniqueName();

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
    }

    outputFile_->Close();
}
