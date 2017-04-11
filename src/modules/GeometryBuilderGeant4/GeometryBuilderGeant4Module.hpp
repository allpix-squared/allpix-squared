/*
 * Geometry construction module
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

class G4RunManager;

namespace allpix {
    // define the module to inherit from the module base class
    class GeometryBuilderGeant4Module : public Module {
    public:
        // provide a static const variable of type string (required!)
        static const std::string name;

        // constructor should take a pointer to the Configuration, the Messenger and the Geometry Manager
        GeometryBuilderGeant4Module(Configuration config, Messenger*, GeometryManager*);
        ~GeometryBuilderGeant4Module() override;

        // build the Geant4 geometry before a run
        void init() override;

    private:
        // configuration for this module
        Configuration config_;

        // link to geometry manager
        GeometryManager* geo_manager_;

        // geant run manager
        // FIXME: is it right to let the geometry own this pointer
        std::unique_ptr<G4RunManager> run_manager_g4_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H */
