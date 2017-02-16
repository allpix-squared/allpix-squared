/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_FACTORY_H
#define ALLPIX_MODULE_FACTORY_H

#include <vector>
#include <memory>

#include "Module.hpp"
#include "../config/Configuration.hpp"

namespace allpix{
    
    class AllPix;
    
    class ModuleFactory{
    public:
        // constructor and destructor
        ModuleFactory() {}
        virtual ~ModuleFactory() {}
        
        // set AllPix object
        void setAllPix(AllPix *);
        AllPix *getAllPix();
        
        // set configuration
        void setConfiguration(Configuration conf);
        Configuration &getConfiguration();
        
        // create a module
        virtual std::vector<std::unique_ptr<Module> > create() = 0;
    private:
        Configuration conf_;
        AllPix *apx_;
    };
}

#endif // ALLPIX_SINGLE_MODULE_FACTORY_H
