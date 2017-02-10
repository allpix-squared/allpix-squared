/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "AllPix.hpp"

using namespace allpix;

// Default constructor
AllPix::AllPix(): conf_mgr_(0), mod_mgr_(0), geo_mgr_(0), msg_(0), state_(State::Unitialized) {}
AllPix::AllPix(ConfigManager *conf_mgr, ModuleManager *mod_mgr, GeometryManager *geo_mgr, Messenger *msg):
    conf_mgr_(conf_mgr), mod_mgr_(mod_mgr), geo_mgr_(geo_mgr), msg_(msg), state_(State::Unitialized) {}
    
// Destructor
AllPix::~AllPix() {
    delete conf_mgr_;
    delete mod_mgr_;
    delete geo_mgr_;
    delete msg_;
}

// Get state
AllPix::State AllPix::getState() const {
    return state_;
}

// Get managers
ConfigManager *AllPix::getConfigManager() {
    return conf_mgr_;
}
GeometryManager *AllPix::getGeometryManager() {
    return geo_mgr_;
}
ModuleManager *AllPix::getModuleManager(){
    return mod_mgr_;
}
Messenger *AllPix::getMessenger(){
    return msg_;
}

// Initialize
void AllPix::init() {
    //TODO: implement
    mod_mgr_->load();
}

void AllPix::run() {
    //TODO: implement
    mod_mgr_->run();
}

void AllPix::finalize() {
    //TODO: implement
    mod_mgr_->finalize();
}
