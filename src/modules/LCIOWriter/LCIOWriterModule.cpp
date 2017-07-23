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

LCIOWriterModule::LCIOWriterModule(Configuration config, Messenger* messenger, GeometryManager*) : Module(config), config_(std::move(config)), pixels_message_(nullptr) {
    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();
    // Bind pixel hits message
    messenger->bindSingle(this, &LCIOWriterModule::pixels_message_, MsgFlags::REQUIRED);

}

void LCIOWriterModule::run(unsigned int) {
    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    LOG(TRACE) << "Running module " << getUniqueName();
}
