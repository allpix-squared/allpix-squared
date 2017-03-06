/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_H
#define ALLPIX_MODULE_H

#include <vector>
#include <string>

#include "ModuleIdentifier.hpp"

#include "../config/Configuration.hpp"

#include "../geometry/Detector.hpp"

namespace allpix {

    class AllPix;
    class Messenger;
    class ModuleManager;
    class GeometryManager;
    
    class Module {
    public:
        // AllPix running state
        enum State{
            // Loaded,
            Ready,
            Running,
            Finished
        };
        
        // Constructor and destructors
        // FIXME: register and unregister the listeners in the constructor?
        Module(AllPix *allpix, ModuleIdentifier identifier);
        virtual ~Module() {}
        
        // FIXME: does it makes sense to copy a module
        Module(const Module&) = delete;
        Module &operator=(const Module&) = delete;
        
        // Modules should have a unique name (for configuration)
        // TODO: depends on implementation how this should work with dynamic loading
        //virtual std::string getName() = 0;
        
        // FIXME: handle by string or directly pass detectors?
        void setDetector(std::shared_ptr<Detector> det) {
            _detector = det;
        }
        std::shared_ptr<Detector> getDetector() const {
            return _detector;
        }
    
        //get the identifier
        ModuleIdentifier getIdentifier() const{
            return identifier_;
        }
        
        // Get access to several services
        // FIXME: use smart pointers here
        AllPix *getAllPix();
        Messenger *getMessenger();
        ModuleManager *getModuleManager();
        GeometryManager *getGeometryManager();
        
        // Initialize the module and pass the configuration etc.
        virtual void init() {
            // FIXME: do default stuff here
        }
        
        // Execute the function of the module
        virtual void run() = 0;
        
        // Finalize module and check if it executed properly 
        // NOTE: useful to do before destruction to allow for exceptions
        virtual void finalize() {
            // FIXME: do default stuff here
        }
        
    private:
        AllPix *allpix_;
        ModuleIdentifier identifier_;
        
        // FIXME: see above
        std::shared_ptr<Detector> _detector;
    };
}

#endif /* ALLPIX_MODULE_MANAGER_H */
