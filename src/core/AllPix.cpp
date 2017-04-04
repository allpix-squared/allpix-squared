/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "AllPix.hpp"

#include <memory>
#include <utility>

#include "core/utils/unit.h"

using namespace allpix;

// FIXME: implement the multi run and general config logic

// default constructor (FIXME: pass the module manager until we have dynamic loading)
AllPix::AllPix(std::string file_name, std::unique_ptr<ModuleManager> mod_mgr)
    : conf_mgr_(std::make_unique<ConfigManager>(std::move(file_name))), mod_mgr_(std::move(mod_mgr)),
      geo_mgr_(std::make_unique<GeometryManager>()), msg_(std::make_unique<Messenger>()) {}

// control methods
void AllPix::init() {
    // set the default units
    add_units();

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

// add all units
void AllPix::add_units() {
    // LENGTH
    Units::add("nm", 1e-6);
    Units::add("um", 1e-3);
    Units::add("mm", 1);
    Units::add("m", 1e3);
    Units::add("km", 1e6);

    // TIME
    Units::add("ps", 1e-3);
    Units::add("ns", 1);
    Units::add("ms", 1e3);
    Units::add("us", 1e6);
    Units::add("s", 1e9);

    // TEMPERATURE
    // FIXME: not really needed as it is the only real option
    Units::add("K", 1);

    // ENERGY
    Units::add("eV", 1e-6);
    Units::add("KeV", 1e-3);
    Units::add("MeV", 1);
    Units::add("GeV", 1e3);

    // CHARGE
    Units::add("C", 1.6021766208e-19);

    // ANGLES
    // NOTE: these are fake units
    Units::add("deg", 0.01745329252);
    Units::add("rad", 1);
}
