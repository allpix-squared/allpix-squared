/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_FACTORY_H
#define ALLPIX_MODULE_FACTORY_H

#include <memory>
#include <vector>

#include "Module.hpp"
#include "core/config/Configuration.hpp"

namespace allpix {

    class AllPix;

    class ModuleFactory {
    public:
        // Constructor and destructor
        ModuleFactory();
        virtual ~ModuleFactory();

        // Disallow copy of a factory
        ModuleFactory(const ModuleFactory&) = delete;
        ModuleFactory& operator=(const ModuleFactory&) = delete;

        // set AllPix object
        void setAllPix(AllPix*);
        AllPix* getAllPix();

        // set configuration
        void setConfiguration(Configuration conf);
        Configuration& getConfiguration();

        // create a module
        virtual std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> create() = 0;

    private:
        Configuration conf_;
        AllPix* apx_;
    };
}

#endif /* ALLPIX_SINGLE_MODULE_FACTORY_H */
