/**
 * Main class for user-defined objects
 */

#ifndef ALLPIX_OBJECT_H
#define ALLPIX_OBJECT_H

#include <TObject.h>

namespace allpix {
    template <typename T> class Message;

    // object definition
    class Object : public TObject {
    public:
        Object();
        ~Object() override;

        Object(const Object&);
        Object& operator=(const Object&);

        Object(Object&&);
        Object& operator=(Object&&);

        ClassDef(Object, 1);
    };
} // namespace allpix

#endif
