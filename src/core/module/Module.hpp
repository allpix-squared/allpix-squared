/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_H
#define ALLPIX_MODULE_H

#include <memory>
#include <string>
#include <vector>

#include "core/geometry/Detector.hpp"

namespace allpix {

    class Messenger;

    class Module {
    public:
        // Constructor and destructors
        explicit Module();
        explicit Module(std::shared_ptr<Detector>);
        virtual ~Module() = default;

        // Disallow copy of a module
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;

        // Modules should have a unique name (for configuration)
        // TODO: depends on implementation how this should work with dynamic loading
        // virtual std::string getName() = 0;

        // Get (possibly) linked detector
        std::shared_ptr<Detector> getDetector();

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
        std::shared_ptr<Detector> detector_ptr__;
    };
  
  class ModuleIdentifier {
  public:
    ModuleIdentifier(std::string module_name, std::string identifier, int prio)
    : name_(std::move(module_name)), identifier_(std::move(identifier)), prio_(prio) {}
    
    // get the name of the module
    std::string getName() const { return name_; }
    // get the unique identifier for this module
    std::string getIdentifier() const { return identifier_; }
    // return the unique name (combination of module and identifier)
    std::string getUniqueName() const { return name_ + identifier_; }
    // get the priority of this unique name
    int getPriority() const { return prio_; }
    
    // operator overloading (NOTE: identifiers are tested only one name - different priorities are equal!)
    bool operator==(const ModuleIdentifier& other) const { return getUniqueName() == other.getUniqueName(); }
    bool operator!=(const ModuleIdentifier& other) const { return getUniqueName() != other.getUniqueName(); }
    bool operator<(const ModuleIdentifier& other) const { return getUniqueName() < other.getUniqueName(); }
    bool operator<=(const ModuleIdentifier& other) const { return getUniqueName() <= other.getUniqueName(); }
    bool operator>(const ModuleIdentifier& other) const { return getUniqueName() > other.getUniqueName(); }
    bool operator>=(const ModuleIdentifier& other) const { return getUniqueName() >= other.getUniqueName(); }
    
  private:
    std::string name_;
    std::string identifier_;
    int prio_;
  };
  
  
}

#endif /* ALLPIX_MODULE_H */
