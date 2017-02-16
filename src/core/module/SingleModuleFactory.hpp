/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_SINGLE_MODULE_FACTORY_H
#define ALLPIX_SINGLE_MODULE_FACTORY_H

#include <vector>
#include <memory>

#include "Module.hpp"
#include "ModuleFactory.hpp"
#include "../config/Configuration.hpp"

// FIXME: ensure proper dynamic deletion

namespace allpix{
    
    template<typename T> class SingleModuleFactory : public ModuleFactory{
    public:
        // create a module
        std::vector<std::unique_ptr<Module> > create(){
            std::vector<std::unique_ptr<Module> > mod_list;
            mod_list.emplace_back(std::make_unique<T>(getAllPix()));
            
            //mod_list.emplace_back(std::make_unique<T>(getAllPix()));
            //FIXME: pass this to the constructor?
            //mod_list.back()->init(getConfiguration());
            return mod_list;
        }
    };
}

#endif // ALLPIX_SINGLE_MODULE_FACTORY_H
