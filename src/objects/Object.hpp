/**
 * @file
 * @brief Definition of Object base class
 * @copyright MIT License
 */

#ifndef ALLPIX_OBJECT_H
#define ALLPIX_OBJECT_H

#include <TObject.h>

namespace allpix {
    template <typename T> class Message;

    /**
     * @brief Base class for internal objects
     */
    class Object : public TObject {
    public:
        /**
         * @brief Required default constructor
         */
        Object() = default;
        /**
         * @brief Required virtual destructor
         */
        ~Object() override = default;

        /// @{
        /**
         * @brief Use default copy behaviour
         */
        Object(const Object&);
        Object& operator=(const Object&);
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        Object(Object&&);
        Object& operator=(Object&&);
        /// @}

        /**
         * @brief ROOT class definition
         */
        ClassDef(Object, 1);
    };
} // namespace allpix

#endif
