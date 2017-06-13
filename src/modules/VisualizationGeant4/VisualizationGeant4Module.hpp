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
    class VisualizationGeant4Module : public Module {
    public:
        // Unique module constructor
        VisualizationGeant4Module(Configuration config, Messenger*, GeometryManager*);
        ~VisualizationGeant4Module() override;

        // Disallow copy
        VisualizationGeant4Module(const VisualizationGeant4Module&) = delete;
        VisualizationGeant4Module& operator=(const VisualizationGeant4Module&) = delete;

        // Default move
        VisualizationGeant4Module(VisualizationGeant4Module&&) noexcept = default;
        VisualizationGeant4Module& operator=(VisualizationGeant4Module&&) noexcept = default;

        // Initializes the visualization and parameters
        void init() override;

        // Show updates every run if not accumulating
        void run(unsigned int) override;

        // Display the visualization
        void finalize() override;

    private:
        void set_visualization_settings();
        void set_visibility_attributes();

        Configuration config_;
        GeometryManager* geo_manager_;

        // variable to check if we did run succesfullly
        bool has_run_;

        // pointer to the visualization manager
        std::unique_ptr<G4VisManager> vis_manager_g4_;

        // sessions and params for the session
        std::string session_param_;
        char* session_param_ptr_;
        std::unique_ptr<G4UIsession> gui_session_;
    };
} // namespace allpix

#endif /* ALLPIX_TEST_VISUALIZATION_MODULE_H */
