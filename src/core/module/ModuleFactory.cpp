/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 *  @author Daniel Hynds <daniel.hynds@cern.ch>
 */

#include "ModuleFactory.hpp"
#include <dlfcn.h>

#include <utility>

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

// Function to create modules from the dynamic library passed from the Module Manager
std::vector<std::pair<ModuleIdentifier, Module*>> ModuleFactory::createModules(std::string moduleName, void* library) {

    // Make the vector to return
    std::vector<std::pair<ModuleIdentifier, Module*>> moduleList;

    // Load an instance of the module from the library
    ModuleIdentifier identifier(moduleName, "", 0);
    Module* module = NULL;

    // Find out if this module should be created once, or once per detector
    bool unique;
    void* uniqueFunction = dlsym(library, "unique");
    char* err = dlerror();
    // If the unique function was not found, throw an error
    if(err != NULL) {
        throw allpix::DynamicLibraryError(moduleName);
    } else {
        unique = reinterpret_cast<bool (*)()>(uniqueFunction)();
    }

    // Get the generator function for this module
    void* generator = dlsym(library, "generator");
    err = dlerror();
    // If the generator function was not found, throw an error
    if(err != NULL) {
        throw allpix::DynamicLibraryError(moduleName);
    } else {
        // Otherwise initialise the module
        module = reinterpret_cast<Module* (*)(Configuration, Messenger*, GeometryManager*)>(generator)(
            getConfiguration(), getMessenger(), getGeometryManager());
    }

    // Store the module and return it to the Module Manager
    moduleList.emplace_back(identifier, module);
    return moduleList;
}

// Function to create modules per detector from the dynamic library passed from the Module Manager
std::vector<std::pair<ModuleIdentifier, Module*>> ModuleFactory::createModulesPerDetector(std::string moduleName,
                                                                                          void* library) {

    // Make the vector to return
    std::set<std::string> moduleNames;
    std::vector<std::pair<ModuleIdentifier, Module*>> moduleList;

    // Open the library and get the module generator function
    void* generator = dlsym(library, "generator");
    char* err = dlerror();
    // If the generator function was not found, throw an error
    if(err != NULL) {
        throw allpix::DynamicLibraryError(moduleName);
    }

    // Pick up the configuration object
    Configuration conf = getConfiguration();

    // FIXME: lot of overlap here...!
    // FIXME: check empty config arrays

    // instantiate all names first with highest priority
    if(conf.has("name")) {
        std::vector<std::string> names = conf.getArray<std::string>("name");
        for(auto& name : names) {
            auto det = getGeometryManager()->getDetector(name);

            // create with detector name and priority
            ModuleIdentifier identifier(moduleName, det->getName(), 1);
            Module* module = reinterpret_cast<Module* (*)(Configuration, Messenger*, std::shared_ptr<Detector>)>(generator)(
                conf, getMessenger(), det);
            moduleList.emplace_back(identifier, module);

            // check if the module called the correct base class constructor
            check_module_detector(identifier.getName(), moduleList.back().second, det.get());
            // save the name (to not override it later)
            moduleNames.insert(name);
        }
    }

    // then instantiate all types that are not yet name instantiated
    if(conf.has("type")) {
        std::vector<std::string> types = conf.getArray<std::string>("type");
        for(auto& type : types) {
            auto detectors = getGeometryManager()->getDetectorsByType(type);

            for(auto& det : detectors) {
                // skip all that were already added by name
                if(moduleNames.find(det->getName()) != moduleNames.end()) {
                    continue;
                }

                // create with detector name and priority
                ModuleIdentifier identifier(moduleName, det->getName(), 2);
                Module* module = reinterpret_cast<Module* (*)(Configuration, Messenger*, std::shared_ptr<Detector>)>(
                    generator)(conf, getMessenger(), det);
                moduleList.emplace_back(identifier, module);

                // check if the module called the correct base class constructor
                check_module_detector(identifier.getName(), moduleList.back().second, det.get());
            }
        }
    }

    // instantiate for all detectors if no name / type provided
    if(!conf.has("type") && !conf.has("name")) {
        auto detectors = getGeometryManager()->getDetectors();

        for(auto& det : detectors) {

            // create with detector name and priority
            ModuleIdentifier identifier(moduleName, det->getName(), 0);
            Module* module = reinterpret_cast<Module* (*)(Configuration, Messenger*, std::shared_ptr<Detector>)>(generator)(
                conf, getMessenger(), det);
            moduleList.emplace_back(identifier, module);

            // check if the module called the correct base class constructor
            check_module_detector(identifier.getName(), moduleList.back().second, det.get());
        }
    }

    return moduleList;
}
