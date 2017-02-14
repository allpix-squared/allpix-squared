/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_H
#define ALLPIX_MODULE_H

#include <vector>
#include <string>

namespace allpix{

    class AllPix;
    
    class Module{
    public:
        // AllPix running state
        enum State{
            //Loaded,
            Ready,
            Running,
            Finished
        };
        
        // Constructor and destructors
        // FIXME: register and unregister the listeners in the constructor?
        Module() {}
        virtual ~Module() {}
        
        // FIXME: does it makes sense to copy a module
        Module(const Module&) = delete;
        Module &operator=(const Module&) = delete;
        
        // Modules should have a unique name (for configuration)
        // TODO: depends on implementation how this should work with dynamic loading
        virtual std::string getName() = 0;
        
        //Initialize the module and pass the configuration etc.
        //FIXME: also depending on constraints possible to do this in the constructor
        virtual void init(AllPix *) = 0;
        
        // Execute the function of the module
        virtual void run() = 0;
        
        // Finalize module and check if it executed properly 
        // NOTE: useful to do before destruction to allow for exceptions
        virtual void finalize() = 0;
    };
}

#endif // ALLPIX_MODULE_MANAGER_H
