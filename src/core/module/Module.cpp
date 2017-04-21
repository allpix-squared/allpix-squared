/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Module.hpp"

#include <memory>
#include <utility>

#include "core/messenger/Messenger.hpp"

using namespace allpix;

Module::Module() : Module(nullptr) {}
Module::Module(std::shared_ptr<Detector> detector) : config_(), detector_(std::move(detector)) {}

// Get the detector
std::shared_ptr<Detector> Module::getDetector() {
    return detector_;
}

// Set internal config
void Module::set_configuration(Configuration config) {
    config_ = std::move(config);
}
Configuration Module::get_configuration() {
    return config_;
}

// Add delegate
void Module::add_delegate(BaseDelegate* delegate) {
    delegates_.emplace_back(delegate);
}
// Reset all delegates
void Module::reset_delegates() {
    for(auto& delegate : delegates_) {
        delegate->reset();
    }
}
