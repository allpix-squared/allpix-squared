/*
 * Geometry construction module
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H

#include <memory>
#include <string>

#include "../../core/geometry/PixelDetectorModel.hpp"
#include "../../core/module/Module.hpp"
#include "../../core/config/Configuration.hpp"

namespace allpix{
    // define the module to inherit from the module base class
    class GeometryConstructionModule : public Module{
    public:
        // provide a static const variable of type string (required!)
        static const std::string name;
        
        // constructor should take a pointer to AllPix, a ModuleIdentifier and a Configuration as input
        GeometryConstructionModule(AllPix *apx, ModuleIdentifier id, Configuration config): Module(apx, id) {
            config_ = config;
        }
        
        // method that will be run where the module should do its computations and possibly dispatch their results as a message
        void run();
        
    private:
        // internal methods
        void buildG4();
        
        // configuration for this module
        Configuration config_;
    };
}

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H */
