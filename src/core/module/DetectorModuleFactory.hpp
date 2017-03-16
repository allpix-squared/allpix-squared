/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DETECTOR_MODULE_FACTORY_H
#define ALLPIX_DETECTOR_MODULE_FACTORY_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Module.hpp"
#include "ModuleFactory.hpp"
#include "ModuleIdentifier.hpp"
#include "core/config/Configuration.hpp"
#include "core/utils/log.h"

// FIXME: ensure proper dynamic deletion

namespace allpix {

    template <typename T> class DetectorModuleFactory : public ModuleFactory {
    public:
        // create a module
        std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> create() override {
            std::set<std::string> all_names;
            std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> mod_list;

            Configuration conf = getConfiguration();

            // FIXME: lot of overlap here...!

            if(conf.has("name")) {
                std::vector<std::string> names = conf.getArray<std::string>("name");
                for(auto& name : names) {
                    auto det = getAllPix()->getGeometryManager()->getDetector(name);

                    ModuleIdentifier identifier(T::name + det->getName(), 1);
                    mod_list.emplace_back(identifier, std::make_unique<T>(getAllPix(), conf));
                    // FIXME: later detector linking
                    mod_list.back().second->setDetector(det);
                    all_names.insert(name);
                }
            }

            if(conf.has("type")) {
                std::vector<std::string> types = conf.getArray<std::string>("type");
                for(auto& type : types) {
                    auto detectors = getAllPix()->getGeometryManager()->getDetectorsByType(type);

                    for(auto& det : detectors) {
                        // skip all that were already added by name
                        if(all_names.find(det->getName()) != all_names.end()) {
                            continue;
                        }

                        ModuleIdentifier identifier(T::name + det->getName(), 2);
                        mod_list.emplace_back(identifier, std::make_unique<T>(getAllPix(), conf));
                        // FIXME: later detector linking
                        mod_list.back().second->setDetector(det);
                    }
                }
            }

            if(!conf.has("type") && !conf.has("name")) {
                auto detectors = getAllPix()->getGeometryManager()->getDetectors();

                for(auto& det : detectors) {
                    ModuleIdentifier identifier(T::name + det->getName(), 0);
                    mod_list.emplace_back(identifier, std::make_unique<T>(getAllPix(), conf));
                    // FIXME: later detector linking
                    mod_list.back().second->setDetector(det);
                }
            }
            // mod_list.emplace_back(std::make_unique<T>(getAllPix()));
            // FIXME: pass this to the constructor?
            // mod_list.back()->init(getConfiguration());
            return mod_list;
        }
    };
}

#endif /* ALLPIX_DETECTOR_MODULE_FACTORY_H */
