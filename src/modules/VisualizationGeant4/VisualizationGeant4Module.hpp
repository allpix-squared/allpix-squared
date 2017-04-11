/*
 * Visualization module
 */

#ifndef ALLPIX_TEST_VISUALIZATION_MODULE_H
#define ALLPIX_TEST_VISUALIZATION_MODULE_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

class G4UIsession;
class G4VisManager;

namespace allpix {
    // define the module to inherit from the module base class
    class VisualizationGeant4Module : public Module {
    public:
        // provide a static const variable of type string (required!)
        static const std::string name;

        // constructor should take a pointer to the Configuration, the Messenger and the Geometry Manager
        VisualizationGeant4Module(Configuration config, Messenger*, GeometryManager*);
        ~VisualizationGeant4Module() override;

        // initializes the visualization and set necessary settings to catch all the required data
        void init() override;

        // displays the visualization
        void finalize() override;

    private:
        // configuration for this module
        Configuration config_;

        // variable to check if we did run succesfullly
        bool has_run_;

        // pointer to the visualization manager
        std::shared_ptr<G4VisManager> vis_manager_g4_;
    };
} // namespace allpix

#endif /* ALLPIX_TEST_VISUALIZATION_MODULE_H */
