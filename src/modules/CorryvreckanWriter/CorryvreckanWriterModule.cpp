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

CorryvreckanWriterModule::CorryvreckanWriterModule(Configuration config, Messenger* messenger, GeometryManager* geoManager)
    : Module(std::move(config)), messenger_(messenger), geometryManager_(geoManager) {

    // Require PixelCharge messages for single detector
    messenger_->bindMulti(this, &CorryvreckanWriterModule::pixel_messages_, MsgFlags::REQUIRED);

    config_.setDefault("file_name", "corryvreckanOutput.root");
    config_.setDefault("geometry_file", "corryvreckanGeometry.conf");
}

// Set up the output trees
void CorryvreckanWriterModule::init() {

    // Create output file and directories
    fileName_ = createOutputFile(allpix::add_file_extension(config_.get<std::string>("file_name"), "root"), true);
    outputFile_ = std::make_unique<TFile>(fileName_.c_str(), "RECREATE");
    outputFile_->cd();
    outputFile_->mkdir("pixels");

    // Create geometry file:
    geometryFileName_ =
        createOutputFile(allpix::add_file_extension(config_.get<std::string>("geometry_file"), "conf"), true);

    // Loop over all detectors and make trees for data
    auto detectors = geometryManager_->getDetectors();
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
            long long int time(time_);
            auto outputPixel = new corryvreckan::Pixel(detectorID, int(pixelY), int(pixelX), int(adc), time);

            LOG(DEBUG) << "Pixel (" << pixelX << "," << pixelY << ") written to device " << detectorID;

            // Map the pixel to the output tree and write it
            treePixels_[objectID] = outputPixel;
            outputTrees_[objectID]->Fill();
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
            throw ModuleError("Cannot write to GEAR geometry file");
        }

        geometry_file << "# Allpix Squared detector geometry - https://cern.ch/allpix-squared/" << std::endl << std::endl;

        for(auto& detector : detectors) {
            geometry_file << "[" << detector->getName() << "]" << std::endl;
            geometry_file << "position = " << Units::display(detector->getPosition().x(), {"mm", "um"}) << ", "
                          << Units::display(detector->getPosition().y(), {"mm", "um"}) << ", "
                          << Units::display(detector->getPosition().z(), {"mm", "um"}) << std::endl;
            ROOT::Math::RotationZYX rotations(detector->getOrientation());
            geometry_file << "orientation = " << Units::display(rotations.Phi(), "deg") << ", "
                          << Units::display(rotations.Psi(), "deg") << ", " << Units::display(rotations.Theta(), "deg")
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
