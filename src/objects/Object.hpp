/**
 * Main class for user-defined objects
 */

#ifndef ALLPIX_OBJECT_H
#define ALLPIX_OBJECT_H

#include <TBuffer.h>

namespace allpix {
    template <typename T> class Message;

    // object definition
    class Object {
    public:
        Object();
        virtual ~Object();

        Object(const Object&);
        Object& operator=(const Object&);

        Object(Object&&);
        Object& operator=(Object&&);

        ClassDef(Object, 1)
    };
} // namespace allpix

inline TBuffer& operator<<(TBuffer& buf, long double d) {
    buf.WriteDouble(static_cast<double>(d));
    return buf;
}
inline TBuffer& operator>>(TBuffer& buf, long double& ld) {
    double d;
    buf.ReadDouble(d);
    ld = static_cast<long double>(d);
    return buf;
}

#endif
