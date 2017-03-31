/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Module.hpp"

#include <memory>
#include <utility>

#include "core/messenger/Messenger.hpp"

using namespace allpix;

Module::Module() : Module(nullptr) {}
Module::Module(std::shared_ptr<Detector> detector) : detector_ptr__(std::move(detector)) {}

// Get the detector
std::shared_ptr<Detector> Module::getDetector() {
    return detector_ptr__;
}

// External function, to allow loading from dynamic library without knowing module type.
// Should be overloaded in all module implementations, added here to prevent crashes
// Module* allpix::generator() {
//    Module* module = new Module(nullptr);
//    return dynamic_cast<Module*>(module);
//}
