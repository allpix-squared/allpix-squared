// include module header
#include "DummyModule.hpp"

// include STL headers
#include <utility>

// include allpix headers
#include <core/utils/log.h>

// use the allpix namespace within this file
using namespace allpix;

// set the name of the module
const std::string DummyModule::name = "<your_module_name>";

// constructor to load the module
DummyModule::DummyModule(AllPix* apx, Configuration config) : Module(apx), config_(std::move(config)) {
    // ... implement ... (typically you want to bind some messages here)
    LOG(DEBUG) << "initializing module " << DummyModule::name;
}

// run method that does the main computations for the module
void DummyModule::run() {
    // ... implement ... (typically you want to fetch some configuration here and in the end possibly output a message)
    LOG(DEBUG) << "running module " << DummyModule::name;
}
