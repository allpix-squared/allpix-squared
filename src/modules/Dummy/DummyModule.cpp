/**
 * @file
 * @brief Implementation of [Dummy] module
 * @copyright MIT License
 */

#include "DummyModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

DummyModule::DummyModule(Configuration config, Messenger*, GeometryManager*) : Module(config), config_(std::move(config)) {
    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();
}

void DummyModule::run(unsigned int) {
    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    LOG(TRACE) << "Running module " << getUniqueName();
}
