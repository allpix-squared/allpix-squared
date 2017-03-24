/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ModuleFactory.hpp"

using namespace allpix;

// Constructor and destructor
ModuleFactory::ModuleFactory() : config_(), messenger_(), geometry_manager_() {}
ModuleFactory::~ModuleFactory() = default;

void ModuleFactory::setConfiguration(Configuration conf) {
    config_ = std::move(conf);
}
Configuration& ModuleFactory::getConfiguration() {
    return config_;
}

void ModuleFactory::setMessenger(Messenger* messenger) {
    messenger_ = messenger;
}
Messenger* ModuleFactory::getMessenger() {
    return messenger_;
}

void ModuleFactory::setGeometryManager(GeometryManager* geo_manager) {
    geometry_manager_ = geo_manager;
}
GeometryManager* ModuleFactory::getGeometryManager() {
    return geometry_manager_;
}
