/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_IDENTIFIER_H
#define ALLPIX_MODULE_IDENTIFIER_H

#include <string>
#include <utility>

namespace allpix {

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

#endif /* ALLPIX_MODULE_IDENTIFIER_H */
