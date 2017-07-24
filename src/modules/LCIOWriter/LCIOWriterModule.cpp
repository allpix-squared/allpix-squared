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
#include <lcio.h>

using namespace allpix;
using namespace lcio;

LCIOWriterModule::LCIOWriterModule(Configuration config, Messenger* messenger, GeometryManager*)
    : Module(config), config_(std::move(config)) {
    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();
    // Bind pixel hits message
    messenger->bindMulti(this, &LCIOWriterModule::pixel_messages_, MsgFlags::REQUIRED);

    lcWriter = LCFactory::getInstance()->createLCWriter();
    lcWriter->open("test.slcio", LCIO::WRITE_NEW);
    LCRunHeaderImpl* run = new LCRunHeaderImpl();
    run->setRunNumber(1);
    run->setDetectorName("telescope");
    lcWriter->writeRunHeader(run);
    delete run;
    // lcWriter->close();
    LOG(STATUS) << "LCIO file writtencd";
}

void LCIOWriterModule::run(unsigned int eventNb) {
    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    LOG(TRACE) << "Running module " << getUniqueName();
    // LOG(DEBUG) << "Adding hits in " << pixels_message_->getData().size() << " pixels";

    LCEventImpl* evt = new LCEventImpl(); // create the event
    evt->setRunNumber(1);
    evt->setEventNumber(static_cast<int>(eventNb)); // set the event attributes

    LCCollectionVec* hitVec = new LCCollectionVec(LCIO::TRACKERDATA);

    for(const auto& hit_msg : pixel_messages_) {
        LOG(DEBUG) << hit_msg->getDetector()->getName();
        for(const auto& hitdata : hit_msg->getData()) {
            LOG(DEBUG) << ", X: " << hitdata.getPixel().getIndex().x() << ", Y:" << hitdata.getPixel().getIndex().y()
                       << ", Signal: " << hitdata.getSignal();

            std::vector<float> charge;
            charge.push_back(static_cast<float>(hitdata.getPixel().getIndex().x()));
            charge.push_back(static_cast<float>(hitdata.getPixel().getIndex().y()));
            charge.push_back(static_cast<float>(hitdata.getSignal()));
            TrackerDataImpl* hit = new TrackerDataImpl();
            hit->setChargeValues(charge);
            hit->setCellID0(0);
            hit->setCellID1(0);
            hitVec->push_back(hit);
        }
    }
    evt->addCollection(hitVec, "original_zsdata"); // add the collection with a name
    lcWriter->writeEvent(evt);                     // write the event to the file
    delete evt;
}

void LCIOWriterModule::finalize() {
    LOG(TRACE) << "Finalizing module " << getUniqueName();
    lcWriter->close();
    delete lcWriter;
}
