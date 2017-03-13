/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DETECTOR_MODULE_FACTORY_H
#define ALLPIX_DETECTOR_MODULE_FACTORY_H

#include <vector>
#include <set>
#include <memory>
#include <string>

#include "Module.hpp"
#include "ModuleFactory.hpp"
#include "../config/Configuration.hpp"
#include "../utils/log.h"

// FIXME: ensure proper dynamic deletion

namespace allpix {

    template<typename T> class DetectorModuleFactory : public ModuleFactory {
    public:
        // create a module
        std::vector<std::unique_ptr<Module> > create() override {
            std::set<std::string> all_names;
            std::vector<std::unique_ptr<Module> > mod_list;

            // FIXME: handle name and type empty case

            Configuration conf = getConfiguration();
            if (conf.has("name")) {
                std::vector<std::string> names = conf.getArray<std::string>("name");
                for (auto &name : names) {
                    // only check now that the detector actually exists
                    auto det = getAllPix()->getGeometryManager()->getDetector(name);

                    ModuleIdentifier identifier(T::name+det.getName(), 1);
                    mod_list.emplace_back(std::make_unique<T>(getAllPix(), identifier, conf));
                    mod_list.back()->setDetector(det);
                    all_names.insert(name);
                }
            }

            if (conf.has("type")) {
                std::vector<std::string> types = conf.getArray<std::string>("type");
                for (auto &type : types) {
                    std::vector<Detector> dets = getAllPix()->getGeometryManager()->getDetectorsByType(type);

                    for (auto &det : dets) {
                        // skip all that were already added by name
                        if (all_names.find(det.getName()) != all_names.end()) continue;

                        ModuleIdentifier identifier(T::name+det.getName(), 2);
                        mod_list.emplace_back(std::make_unique<T>(getAllPix(), identifier, conf));
                        mod_list.back()->setDetector(det);
                    }
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
