/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Module.hpp"

#include "../AllPix.hpp"
#include "../config/ConfigManager.hpp"
#include "../geometry/GeometryManager.hpp"
#include "../messenger/Messenger.hpp"

using namespace allpix;

Module::Module(AllPix* allpix, ModuleIdentifier identifier)
    : allpix_(allpix), identifier_(std::move(identifier)), _detector(nullptr) {}

AllPix* Module::getAllPix() { return allpix_; }

Messenger* Module::getMessenger() { return allpix_->getMessenger(); }

ModuleManager* Module::getModuleManager() { return allpix_->getModuleManager(); }

GeometryManager* Module::getGeometryManager() { return allpix_->getGeometryManager(); }
