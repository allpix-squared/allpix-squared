/*
 * Deposition module
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

class G4UserLimits;
class G4RunManager;

namespace allpix {
    // define the module to inherit from the module base class
    class DepositionGeant4Module : public Module {
    public:
        // constructor should take a pointer to the Configuration, the Messenger and the Geometry Manager
        DepositionGeant4Module(Configuration, Messenger*, GeometryManager*);
        ~DepositionGeant4Module() override;

        // initialize the physics list and the generators
        void init() override;

        // run a single deposition
        void run() override;

    private:
        // configuration for this module
        Configuration config_;

        // messenger to emit deposits
        Messenger* messenger_;

        // global geometry manager
        GeometryManager* geo_manager_;

        // G4 user step limits we should manage
        std::unique_ptr<G4UserLimits> user_limits_;
        // pointer to the Geant4 run manager (owned by other module...)
        G4RunManager* run_manager_g4_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_H */
