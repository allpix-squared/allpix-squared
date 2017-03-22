/*
 * Deposition module
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    // define the module to inherit from the module base class
    class DepositionGeant4Module : public Module {
    public:
        // provide a static const variable of type string (required!)
        static const std::string name;

        // constructor should take a pointer to AllPix, a ModuleIdentifier and a Configuration as input
        DepositionGeant4Module(AllPix* apx, Configuration config);
        ~DepositionGeant4Module() override;

        // method that will be run where the module should do its computations and possibly dispatch their results as a
        // message
        void run() override;

    private:
        // configuration for this module
        Configuration config_;
    };
}

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_H */
