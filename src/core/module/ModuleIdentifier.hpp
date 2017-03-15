/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_IDENTIFIER_H
#define ALLPIX_MODULE_IDENTIFIER_H

#include <string>

namespace allpix {

    class ModuleIdentifier {
    public:
        ModuleIdentifier(std::string identifier, int prio) : identifier_(std::move(identifier)), prio_(prio) {}

        // name and priority
        std::string getUniqueName() const { return identifier_; }
        int getPriority() const { return prio_; }
        
        // operator overloading (NOTE: identifiers are tested only one name - different priorities are equal!)
        bool operator ==(const ModuleIdentifier& other) const { return identifier_ == other.identifier_; }
        bool operator !=(const ModuleIdentifier& other) const { return identifier_ != other.identifier_; }
        bool operator <(const ModuleIdentifier& other) const  { return identifier_ < other.identifier_; }
        bool operator <=(const ModuleIdentifier& other) const { return identifier_ <= other.identifier_; }
        bool operator >(const ModuleIdentifier& other) const { return identifier_ > other.identifier_; }
        bool operator >=(const ModuleIdentifier& other) const { return identifier_ >= other.identifier_; }

    private:
        std::string identifier_;
        int         prio_;
    };
}

#endif /* ALLPIX_MODULE_IDENTIFIER_H */
