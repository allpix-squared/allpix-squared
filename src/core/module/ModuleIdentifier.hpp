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

        std::string getUniqueName() { return identifier_; }

        int getPriority() { return prio_; }

    private:
        std::string identifier_;
        int         prio_;
    };
}

#endif /* ALLPIX_MODULE_IDENTIFIER_H */
