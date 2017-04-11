/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "AllPix.hpp"

#include <memory>
#include <utility>

#include "core/config/InvalidValueError.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

// FIXME: implement the multi run and general config logic

// default constructor (FIXME: pass the module manager until we have dynamic loading)
AllPix::AllPix(std::string file_name, std::unique_ptr<ModuleManager> mod_mgr)
    : conf_mgr_(std::make_unique<ConfigManager>(std::move(file_name))), mod_mgr_(std::move(mod_mgr)),
      geo_mgr_(std::make_unique<GeometryManager>()), msg_(std::make_unique<Messenger>()) {}

// load all modules and the allpix default configuration
void AllPix::load() {
    // add the standard special sections
    conf_mgr_->addGlobalHeaderName("");
    conf_mgr_->addGlobalHeaderName("AllPix");
    conf_mgr_->addIgnoreHeaderName("Ignore");

    // set the log level from config
    Configuration global_config = conf_mgr_->getGlobalConfiguration();
    std::string log_level_string = global_config.get<std::string>("log_level", "INFO");
    try {
        LogLevel log_level = Log::getLevelFromString(log_level_string);
        Log::setReportingLevel(log_level);
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(global_config, "log_level", e.what());
    }
    LOG(DEBUG) << "Setting log level to " << log_level_string;

    // set the default units to use
    add_units();

    // load the modules
    LOG(DEBUG) << "Loading modules";
    mod_mgr_->load(msg_.get(), conf_mgr_.get(), geo_mgr_.get());
    LOG(DEBUG) << "Modules succesfully loaded"; // FIXME: add some more info
}

// init / run / finalize control methods
void AllPix::init() {
    LOG(DEBUG) << "Initializing modules";
    // initialize all modules
    mod_mgr_->init();
    LOG(DEBUG) << "Finished initialization";
}
void AllPix::run() {
    LOG(DEBUG) << "Running modules";
    // run the modules
    mod_mgr_->run();
    LOG(DEBUG) << "Finished run";
}
void AllPix::finalize() {
    LOG(DEBUG) << "Finalizing modules";
    // finalize the modules
    mod_mgr_->finalize();
    LOG(DEBUG) << "Finalization completed";
}

// add all units
void AllPix::add_units() {
    // LENGTH
    Units::add("nm", 1e-6);
    Units::add("um", 1e-3);
    Units::add("mm", 1);
    Units::add("cm", 1e1);
    Units::add("dm", 1e2);
    Units::add("m", 1e3);
    Units::add("km", 1e6);

    // TIME
    Units::add("ps", 1e-3);
    Units::add("ns", 1);
    Units::add("us", 1e3);
    Units::add("ms", 1e6);
    Units::add("s", 1e9);

    // TEMPERATURE
    Units::add("K", 1);

    // ENERGY
    Units::add("eV", 1e-6);
    Units::add("keV", 1e-3);
    Units::add("MeV", 1);
    Units::add("GeV", 1e3);

    // CHARGE
    Units::add("e", 1);
    Units::add("C", 1.6021766208e-19);

    // VOLTAGE
    // NOTE: fixed by above
    Units::add("V", 1e-6);
    Units::add("kV", 1e-3);

    // ANGLES
    // NOTE: these are fake units
    Units::add("deg", 0.01745329252);
    Units::add("rad", 1);
}
