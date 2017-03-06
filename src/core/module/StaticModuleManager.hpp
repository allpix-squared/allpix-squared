/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_STATIC_MODULE_MANAGER_H
#define ALLPIX_STATIC_MODULE_MANAGER_H

#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <string>

#include "Module.hpp"
#include "ModuleManager.hpp"
#include "ModuleFactory.hpp"

namespace allpix {

    class AllPix;
    
    class StaticModuleManager : public ModuleManager{
    public:
        using GeneratorFunction = std::function<std::unique_ptr<ModuleFactory>(std::string)>;
        
        // Constructor and destructors
        explicit StaticModuleManager(GeneratorFunction);
        
        // Load modules
        void load(AllPix *);
        
    private:
        std::unique_ptr<ModuleFactory> get_factory(std::string name);
        
        std::map<std::string, int> _instantiations_map;
                
        GeneratorFunction generator_func_;
    };
}

#endif /* ALLPIX_STATIC_MODULE_MANAGER_H */
