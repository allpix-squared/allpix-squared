/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Module.hpp"

#include "../AllPix.hpp"
#include "../messenger/Messenger.hpp"
#include "../geometry/GeometryManager.hpp"
#include "../config/ConfigManager.hpp"

using namespace allpix;

Module::Module(AllPix *allpix, ModuleIdentifier identifier):
    allpix_(allpix), identifier_(std::move(identifier)), _detector(nullptr) {}

AllPix *Module::getAllPix() {
    return allpix_;
}

Messenger *Module::getMessenger() {
    return allpix_->getMessenger();
}

ModuleManager *Module::getModuleManager() {
    return allpix_->getModuleManager();
}

GeometryManager *Module::getGeometryManager() {
    return allpix_->getGeometryManager();
}
