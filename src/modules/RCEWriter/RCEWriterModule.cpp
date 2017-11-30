/**
 * @file
 * @brief Implementation of RCE Writer Module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "RCEWriterModule.hpp"

#include <string>
#include <utility>

#include <TBranchElement.h>
#include <TClass.h>
#include <TDirectory.h>

#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "objects/Object.hpp"
#include "objects/objects.h"

using namespace allpix;

RCEWriterModule::RCEWriterModule(Configuration config, Messenger* messenger, GeometryManager* geo_mgr)
    : Module(std::move(config)), geo_mgr_(geo_mgr) {
    // Bind to PixelHitMessage
    messenger->bindMulti(this, &RCEWriterModule::pixel_hit_messages_);
}
RCEWriterModule::~RCEWriterModule() = default;

void RCEWriterModule::init() {
    // Create output file
    std::string file_name =
        createOutputFile(allpix::add_file_extension(config_.get<std::string>("file_name", "rce_data"), "root"));
    output_file_ = std::make_unique<TFile>(file_name.c_str(), "RECREATE");
    output_file_->cd();

    // Initialize the events tree
    event_tree_ = std::make_unique<TTree>("Event", "");
    event_tree_->Branch("TimeStamp", &timestamp_);
    event_tree_->Branch("FrameNumber", &frame_number_);
    event_tree_->Branch("TriggerOffset", &trigger_offset_);
    event_tree_->Branch("TriggerInfo", &trigger_info_);
    event_tree_->Branch("TriggerTime", &trigger_time_);
    event_tree_->Branch("Invalid", &invalid_);

    // Get the detector names
    for(const auto& detector : geo_mgr_->getDetectors()) {
        detector_names_.push_back(detector->getName());
    }
    // Sort the detector names
    std::sort(detector_names_.begin(), detector_names_.end());
    // For each detector name, initialze an instance of SensorData
    int det_index = 0;
    for(const auto& detector_name : detector_names_) {
        auto& sensor = sensors_[detector_name];
        // Create directories for each detector
        det_dir_name = "Plane" + std::to_string(det_index);
        TDirectory* detector = output_file_->mkdir(det_dir_name.c_str());
        detector->cd();
        det_index += 1;

        // Initialize the struct for each detector
        sensor.tree = std::make_unique<TTree>("Hits", "");
        LOG(TRACE) << "Detector name is: " << detector_name;
        // initialze tree branches for each instance of the sensorData
        sensor.tree->Branch("NHits", &sensor.nhits_);
        sensor.tree->Branch("PixX", &sensor.pix_x_, "PixX[NHits]/I");
        sensor.tree->Branch("PixY", &sensor.pix_y_, "PixY[NHits]/I");
        sensor.tree->Branch("Value", &sensor.value_, "Value[NHits]/I");
        sensor.tree->Branch("Timing", &sensor.timing_, "Timing[NHits]/I");
        sensor.tree->Branch("HitInCluster", &sensor.hit_in_cluster_, "HitInCluster[NHits]/I");
    }
}

void RCEWriterModule::run(unsigned int event_id) {
    // fill per-event data
    timestamp_ = 0;
    frame_number_ = event_id;
    trigger_offset_ = 0;
    trigger_info_ = 0;
    trigger_time_ = 0;
    invalid_ = false;

    LOG(TRACE) << "Writing new objects to the Events tree";
    // Fill the events tree
    event_tree_->Fill();

    // Loop over all the detectors
    for(const auto& detector_name : detector_names_) {
        // reset nhits
        auto& sensor = sensors_[detector_name];
        sensor.nhits_ = 0;
    }

    // Loop over the pixel hit messages
    for(const auto& hit_msg : pixel_hit_messages_) {

        std::string detector_name = hit_msg->getDetector()->getName();
        auto& sensor = sensors_[detector_name];

        // Loop over all the hits
        for(const auto& hit : hit_msg->getData()) {
            int i = sensor.nhits_;

            // Fill the tree with received messages
            sensor.nhits_ += 1;
            sensor.pix_x_[i] = static_cast<Int_t>(hit.getPixel().getIndex().x()); // NOLINT
            sensor.pix_y_[i] = static_cast<Int_t>(hit.getPixel().getIndex().y()); // NOLINT
            sensor.value_[i] = static_cast<Int_t>(hit.getSignal());               // NOLINT
            // Set the  Timing and HitInCluster for each sesnor_tree (= 0 for now)
            sensor.timing_[i] = 0;         // NOLINT
            sensor.hit_in_cluster_[i] = 0; // NOLINT

            LOG(TRACE) << "Detector Name: " << detector_name << ", X: " << hit.getPixel().getIndex().x()
                       << ", Y:" << hit.getPixel().getIndex().y() << ", Signal: " << hit.getSignal();
        }
    }

    // Loop over all the detectors to fill all corresponding sensor trees
    for(const auto& detector_name : detector_names_) {
        LOG(TRACE) << "Writing new objects to the Sensor Tree for " << detector_name;
        sensors_[detector_name].tree->Fill();
    }
}

void RCEWriterModule::finalize() {
    LOG(TRACE) << "Writing objects to file";
    // Finish writing to the output file
    output_file_->Write();
}
