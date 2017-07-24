/**
 * @file
 * @brief Implementation of [LCIOWriter] module
 * @copyright MIT License
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
    : Module(config), config_(std::move(config)) {
    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();
    // Bind pixel hits message
    messenger->bindMulti(this, &LCIOWriterModule::pixel_messages_, MsgFlags::REQUIRED);

    // get all detector names and assign id.
    std::vector<std::shared_ptr<Detector>> detectors = geo->getDetectors();
    unsigned int i = 0;
    for(const auto& det : detectors) {
        detectorIDs[det->getName()] = i;
        LOG(DEBUG) << det->getName() << " has ID " << detectorIDs[det->getName()];
        i++;
    }

    // Open LCIO file and write run header
    lcWriter = LCFactory::getInstance()->createLCWriter();
    lcWriter->open(config_.get<std::string>("file_name", "output.slcio"), LCIO::WRITE_NEW);
    LCRunHeaderImpl* run = new LCRunHeaderImpl();
    run->setRunNumber(1);
    run->setDetectorName("telescope");
    lcWriter->writeRunHeader(run);
    delete run;
    LOG(DEBUG) << "LCIO run header written";
}

void LCIOWriterModule::run(unsigned int eventNb) {

    int _pixelType = 2;

    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    LOG(TRACE) << "Running module " << getUniqueName();
    // LOG(DEBUG) << "Adding hits in " << pixels_message_->getData().size() << " pixels";

    LCEventImpl* evt = new LCEventImpl(); // create the event
    evt->setRunNumber(1);
    evt->setEventNumber(static_cast<int>(eventNb)); // set the event attributes

    // Prepare charge vectors
    std::vector<std::vector<float>> charges;
    for(unsigned int i = 0; i < detectorIDs.size(); i++) {
        std::vector<float> charge;
        charges.push_back(charge);
    }

    // Receive all pixel messages, fill charge vectors
    for(const auto& hit_msg : pixel_messages_) {
        LOG(DEBUG) << hit_msg->getDetector()->getName();
        for(const auto& hitdata : hit_msg->getData()) {
            LOG(DEBUG) << "X: " << hitdata.getPixel().getIndex().x() << ", Y:" << hitdata.getPixel().getIndex().y()
                       << ", Signal: " << hitdata.getSignal();

            unsigned int detectorID = detectorIDs[hit_msg->getDetector()->getName()];

            charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
            charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
            charges[detectorID].push_back(static_cast<float>(hitdata.getSignal()));               // signal
            charges[detectorID].push_back(static_cast<float>(0));                                 // time
        }
    }

    // Prepare hitvector
    LCCollectionVec* hitVec = new LCCollectionVec(LCIO::TRACKERDATA);

    // Fill hitvector with event data
    for(unsigned int detectorID = 0; detectorID < detectorIDs.size(); detectorID++) {
        TrackerDataImpl* hit = new TrackerDataImpl();
        CellIDEncoder<TrackerDataImpl> sparseDataEncoder("sensorID:7,sparsePixelType:5", hitVec);
        sparseDataEncoder["sensorID"] = detectorID;
        sparseDataEncoder["sparsePixelType"] = static_cast<int>(_pixelType);
        sparseDataEncoder.setCellID(hit);
        hit->setChargeValues(charges[detectorID]);
        hitVec->push_back(hit);
    }

    // Add collection to event and write event to LCIO file
    evt->addCollection(hitVec, "original_zsdata"); // add the collection with a name
    lcWriter->writeEvent(evt);                     // write the event to the file
    delete evt;
}

void LCIOWriterModule::finalize() {
    LOG(TRACE) << "Finalizing module " << getUniqueName();
    lcWriter->close();
    delete lcWriter;
}
