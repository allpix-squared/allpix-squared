/**
 * @file
 * @brief Implementation of [LCIOWriter] module
 * @copyright MIT License
 */

#include "LCIOWriterModule.hpp"

#include <string>
#include <utility>

#include "core/messenger/Messenger.hpp"

#include "core/utils/log.h"

using namespace allpix;

LCIOWriterModule::LCIOWriterModule(Configuration config, Messenger* messenger, GeometryManager*) : Module(config), config_(std::move(config)) {
    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();
    // Bind pixel hits message
    messenger->bindMulti(this, &LCIOWriterModule::pixel_messages_, MsgFlags::REQUIRED);

}

void LCIOWriterModule::run(unsigned int) {
    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    LOG(TRACE) << "Running module " << getUniqueName();
    //LOG(DEBUG) << "Adding hits in " << pixels_message_->getData().size() << " pixels";

    for (const auto& hit_msg : pixel_messages_) {
      LOG(DEBUG)<<hit_msg->getDetector()->getName();
      for (const auto& hit : hit_msg->getData()){
            LOG(DEBUG) << ", X: " << hit.getPixel().getIndex().x()
                       << ", Y:" << hit.getPixel().getIndex().y() << ", Signal: " << hit.getSignal();

      }
    }
}
