/**
 * @file
 * @brief Implementation of [LCIOWriter] module
 * @copyright MIT License
 */

#include "LCIOWriterModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

LCIOWriterModule::LCIOWriterModule(Configuration config, Messenger*, GeometryManager*) : Module(config), config_(std::move(config)) {
    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();
}

void LCIOWriterModule::run(unsigned int) {
    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    LOG(TRACE) << "Running module " << getUniqueName();
}
