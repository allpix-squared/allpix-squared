/*
 * Minimal dummy module to use as a start for the development of your own module
 *
 * To integrate your own module into the framework, follow the steps below
 * - copy this module to a new directory <your_directory_name> in modules
 * - modify CMakeLists.txt in the parent modules directory and add option_build_module(<your_directory_name>)
 * - (temporary) add an include to the module .hpp file to src/exec/allpix.cpp
 * - (temporary) add an if statement with initializer to the generator() function in src/exec/allpix.cpp (see others)
 * - (temporary) link the module by adding target_link_libraries(allpix <your_directory_name>) to src/exec/CMakeLists.txt
 */

// include stl
#include <string>

// include dependencies
#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"

// include module base class
#include <core/module/Module.hpp>

// use the allpix namespace
namespace allpix {
    // define the module inheriting from the module base class
    class DummyModule : public Module {
    public:
        // provide a static const variable of type string to hold the name of the module (required!)
        static const std::string name;

        // constructor should take a pointer to the Configuration, the Messenger and the Geometry Manager
        DummyModule(Configuration config, Messenger*, GeometryManager*);

        // method that will do the computations
        void run();

    private:
        Configuration config_;
    };
}
