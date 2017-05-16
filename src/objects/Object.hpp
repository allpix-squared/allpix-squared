/**
 * Main class for user-defined objects
 */

#ifndef ALLPIX_OBJECT_H
#define ALLPIX_OBJECT_H

#include <Math/DisplacementVector2D.h>

#include "core/messenger/Message.hpp"

#include "Object.hpp"

namespace allpix {
    // object definition
    class Object {
    public:
        Object();
        virtual ~Object();

        Object(const Object&);
        Object& operator=(const Object&);

        Object(Object&&) noexcept;
        Object& operator=(Object&&) noexcept;
    };
} // namespace allpix

#endif
