/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "AllPix.hpp"

#include <utility>
#include <memory>

using namespace allpix;

// Default constructor (FIXME: decide what to pass and what should be standard)
AllPix::AllPix(std::unique_ptr<ConfigManager> conf_mgr, std::unique_ptr<ModuleManager> mod_mgr, std::unique_ptr<GeometryManager> geo_mgr):
    conf_mgr_(std::move(conf_mgr)), mod_mgr_(std::move(mod_mgr)), geo_mgr_(std::move(geo_mgr)), state_(State::Unitialized) {
        msg_ = std::make_unique<Messenger>();
    }

// Get state
AllPix::State AllPix::getState() const {
    return state_;
}

// Get managers
ConfigManager *AllPix::getConfigManager() {
    return conf_mgr_.get();
}
GeometryManager *AllPix::getGeometryManager() {
    return geo_mgr_.get();
}
ModuleManager *AllPix::getModuleManager() {
    return mod_mgr_.get();
}
Messenger *AllPix::getMessenger() {
    return msg_.get();
}

// Initialize
void AllPix::init() {
    // TODO: implement
    mod_mgr_->load(this);
}

void AllPix::run() {
    // TODO: implement
    mod_mgr_->run();
}

void AllPix::finalize() {
    // TODO: implement
    mod_mgr_->finalize();
}
