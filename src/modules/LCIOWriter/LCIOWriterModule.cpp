/**
 * @file
 * @brief Implementation of [LCIOWriter] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "LCIOWriterModule.hpp"

#include <string>
#include <utility>
#include <vector>

#include "core/messenger/Messenger.hpp"

#include "core/utils/log.h"

#include <IMPL/LCCollectionVec.h>
#include <IMPL/LCEventImpl.h>
#include <IMPL/LCRunHeaderImpl.h>
#include <IMPL/TrackerDataImpl.h>
#include <IO/LCWriter.h>
#include <IOIMPL/LCFactory.h>
#include <UTIL/CellIDEncoder.h>
#include <lcio.h>

using namespace allpix;
using namespace lcio;

LCIOWriterModule::LCIOWriterModule(Configuration config, Messenger* messenger, GeometryManager* geo)
    : Module(std::move(config)) {

    // Bind pixel hits message
    messenger->bindMulti(this, &LCIOWriterModule::pixel_messages_, MsgFlags::REQUIRED);

    // get all detector names and assign id.
    std::vector<std::shared_ptr<Detector>> detectors = geo->getDetectors();
    unsigned int i = 0;
    for(const auto& det : detectors) {
        detectorIDs_[det->getName()] = i;
        LOG(DEBUG) << det->getName() << " has ID " << detectorIDs_[det->getName()];
        i++;
    }

    // Set configuration defaults:
    config_.setDefault("file_name", "output.slcio");
    config_.setDefault("pixel_type", 2);
    config_.setDefault("detector_name", "EUTelescope");
    config_.setDefault("output_collection_name", "zsdata_m26");

    pixelType_ = config_.get<int>("pixel_type");
    DetectorName_ = config_.get<std::string>("detector_name");
    OutputCollectionName_ = config_.get<std::string>("output_collection_name");

    // Open LCIO file and write run header
    lcWriter_ = LCFactory::getInstance()->createLCWriter();
    lcWriter_->open(config_.get<std::string>("file_name"), LCIO::WRITE_NEW);
    auto run = std::make_unique<LCRunHeaderImpl>();
    run->setRunNumber(1);
    run->setDetectorName(DetectorName_);
    lcWriter_->writeRunHeader(run.get());
}

void LCIOWriterModule::run(unsigned int eventNb) {

    auto evt = std::make_unique<LCEventImpl>(); // create the event
    evt->setRunNumber(1);
    evt->setEventNumber(static_cast<int>(eventNb)); // set the event attributes
    evt->parameters().setValue("EventType", 2);

    // Prepare charge vectors
    std::vector<std::vector<float>> charges;
    for(unsigned int i = 0; i < detectorIDs_.size(); i++) {
        std::vector<float> charge;
        charges.push_back(charge);
    }

    // Receive all pixel messages, fill charge vectors
    for(const auto& hit_msg : pixel_messages_) {
        LOG(DEBUG) << hit_msg->getDetector()->getName();
        for(const auto& hitdata : hit_msg->getData()) {
            LOG(DEBUG) << "X: " << hitdata.getPixel().getIndex().x() << ", Y:" << hitdata.getPixel().getIndex().y()
                       << ", Signal: " << hitdata.getSignal();

            unsigned int detectorID = detectorIDs_[hit_msg->getDetector()->getName()];

            switch(pixelType_) {
            case 1: // EUTelSimpleSparsePixel
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[detectorID].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                break;
            case 2:  // EUTelGenericSparsePixel
            default: // EUTelGenericSparsePixel is default
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[detectorID].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                break;
            case 5: // EUTelTimepix3SparsePixel
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[detectorID].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                break;
            }
        }
    }

    // Prepare hitvector
    LCCollectionVec* hitVec = new LCCollectionVec(LCIO::TRACKERDATA);

    // Fill hitvector with event data
    for(unsigned int detectorID = 0; detectorID < detectorIDs_.size(); detectorID++) {
        auto hit = new TrackerDataImpl();
        CellIDEncoder<TrackerDataImpl> sparseDataEncoder("sensorID:7,sparsePixelType:5", hitVec);
        sparseDataEncoder["sensorID"] = detectorID;
        sparseDataEncoder["sparsePixelType"] = pixelType_;
        sparseDataEncoder.setCellID(hit);
        hit->setChargeValues(charges[detectorID]);
        hitVec->push_back(hit);
    }

    // Add collection to event and write event to LCIO file
    evt->addCollection(hitVec, OutputCollectionName_); // add the collection with a name
    lcWriter_->writeEvent(evt.get());                  // write the event to the file
}

void LCIOWriterModule::finalize() {
    lcWriter_->close();
}

LCIOWriterModule::~LCIOWriterModule() {
    delete lcWriter_;
}
