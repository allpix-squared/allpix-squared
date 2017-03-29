/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_ERROR_H
#define ALLPIX_MODULE_ERROR_H

#include "exceptions.h"

namespace allpix {
    /*
     * General exceptions for modules if something goes wrong (called by modules)
     */
    class ModuleError : public RuntimeError {
    public:
        explicit ModuleError(std::string reason) { error_message_ = std::move(reason); }
    };
}

#endif /* ALLPIX_MODULE_ERROR_H */
