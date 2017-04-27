// include module header
#include "DummyModule.hpp"

// include STL headers
#include <string>
#include <utility>

// include allpix headers
#include " core/utils/log.h"

// use the allpix namespace within this file
using namespace allpix;

// constructor to load the module
DummyModule::DummyModule(Configuration config, Messenger*, GeometryManager*) : config_(std::move(config)) {
    // ... implement ... (typically you want to bind some messages here)
    LOG(DEBUG) << "initializing module " << DummyModule::name;
}

// run method that does the main computations for the module
void DummyModule::run(unsigned int) {
    // ... implement ... (typically you want to fetch some configuration here and in the end possibly output a message)
    LOG(DEBUG) << "running module " << DummyModule::name;
}
