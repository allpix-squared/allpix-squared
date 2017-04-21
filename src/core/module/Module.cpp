/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Module.hpp"

#include <memory>
#include <utility>

#include "core/messenger/Messenger.hpp"

using namespace allpix;

Module::Module() : Module(nullptr) {}
Module::Module(std::shared_ptr<Detector> detector)
    : identifier_(), config_(), delegates_(), detector_(std::move(detector)) {}

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

// Set internal identifier
void Module::set_identifier(ModuleIdentifier identifier) {
    identifier_ = std::move(identifier);
}
ModuleIdentifier Module::get_identifier() {
    return identifier_;
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
// Check if all delegates are satisfied
bool Module::check_delegates() {
    for(auto& delegate : delegates_) {
        // Return if any delegate is not satisfied
        if(!delegate->isSatisfied()) {
            return false;
        }
    }
    return true;
}
