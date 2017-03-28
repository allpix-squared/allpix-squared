/*
 * Deposition module
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_H

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    // define the module to inherit from the module base class
    class DepositionGeant4Module : public Module {
    public:
        // provide a static const variable of type string (required!)
        static const std::string name;

        // constructor should take a pointer to the Configuration, the Messenger and the Geometry Manager
        DepositionGeant4Module(Configuration, Messenger*, GeometryManager*);
        ~DepositionGeant4Module() override;

        // method that will be run where the module should do its computations and possibly dispatch their results as a
        // message
        void run() override;

    private:
        // configuration for this module
        Configuration config_;

        // messenger to emit deposits
        Messenger* messenger_;

        // global geometry manager
        GeometryManager* geo_manager_;
    };
}

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_H */
