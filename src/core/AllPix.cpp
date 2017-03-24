/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "AllPix.hpp"

#include <memory>
#include <utility>

using namespace allpix;

// FIXME: implement the multi run and general config logic

// default constructor (FIXME: pass the module manager until we have dynamic loading)
AllPix::AllPix(std::string file_name, std::unique_ptr<ModuleManager> mod_mgr) : mod_mgr_(std::move(mod_mgr)) {
    conf_mgr_ = std::make_unique<ConfigManager>(std::move(file_name));
    msg_ = std::make_unique<Messenger>();
    geo_mgr_ = std::make_unique<GeometryManager>();
}

void AllPix::init() {
    // load the modules
    mod_mgr_->load(msg_.get(), conf_mgr_.get(), geo_mgr_.get());
}

void AllPix::run() {
    // run the modules
    mod_mgr_->run();
}

void AllPix::finalize() {
    // finalize the modules
    mod_mgr_->finalize();
}
