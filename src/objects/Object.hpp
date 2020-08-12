/**
 * @file
 * @brief Definition of Object base class
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

/**
 * @defgroup Objects Objects
 * @brief Collection of objects passed around between modules
 */

#ifndef ALLPIX_OBJECT_H
#define ALLPIX_OBJECT_H

#include <iostream>

#include <TObject.h>
#include <TRef.h>

namespace allpix {
    template <typename T> class Message;

    /**
     * @ingroup Objects
     * @brief Base class for internal objects
     */
    class Object : public TObject {
    public:
        friend std::ostream& operator<<(std::ostream& out, const allpix::Object& obj);

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
        Object(const Object&) = default;
        Object& operator=(const Object&) = default;
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        Object(Object&&) = default;
        Object& operator=(Object&&) = default;
        /// @}

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(Object, 3);

        virtual void petrifyHistory() = 0;

    protected:
        /**
         * @brief Print an ASCII representation of this Object to the given stream
         * @param out Stream to print to
         */
        virtual void print(std::ostream& out) const { out << "<unknown object>"; };

        /**
         * @brief Override function to implement ROOT Print()
         * @warning Should not be used inside the framework but might assist in inspecting ROOT files with these objects.
         */
        void Print(Option_t*) const override {
            print(std::cout);
            std::cout << std::endl;
        }

    public:
        template <class T> class PointerWrapper {
        public:
            PointerWrapper() = default;
            PointerWrapper(const T* obj) : ptr_(reinterpret_cast<uintptr_t>(obj)) {}
            virtual ~PointerWrapper() = default;

            /// @{
            /**
             * @brief Use default copy behaviour
             */
            PointerWrapper(const PointerWrapper& rhs) = default;
            PointerWrapper& operator=(const PointerWrapper& rhs) = default;
            /// @}

            /// @{
            /**
             * @brief Use default move behaviour
             */
            PointerWrapper(PointerWrapper&& rhs) = default;
            PointerWrapper& operator=(PointerWrapper&& rhs) = default;
            /// @}

            T* get() const {
                if(ptr_ == 0) {
                    ptr_ = reinterpret_cast<uintptr_t>(ref_.GetObject());
                }

                return reinterpret_cast<T*>(ptr_);
            };

            void store() {
                ref_ = reinterpret_cast<T*>(ptr_);
                ptr_ = 0;
            }

            ClassDef(PointerWrapper, 1);

        private:
            mutable uintptr_t ptr_{};
            TRef ref_{};
        };
    };

    /**
     * @brief Overloaded ostream operator for printing of object data
     * @param out Stream to write output to
     * @param obj Object to print to stream
     * @return Stream where output was written to
     */
    std::ostream& operator<<(std::ostream& out, const allpix::Object& obj);
} // namespace allpix

/**
 * @brief Missing comparison operator for TRef (to filter out unique)
 * @param ref1 First TRef
 * @param ref2 Second TRef
 * @return bool Returns true if first TRef should be ordered before second TRef
 */
bool operator<(const TRef& ref1, const TRef& ref2);

#endif /* ALLPIX_OBJECT_H */
