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
#include <mutex>

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
        ClassDefOverride(Object, 3); // NOLINT

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
        template <class T> class BaseWrapper {
        public:
            /**
             * @brief Required default constructor
             */
            BaseWrapper() = default;
            /**
             * @brief Constructor with object pointer to be wrapped
             * @param obj Pointer to object
             */
            explicit BaseWrapper(const T* obj) : ptr_(const_cast<T*>(obj)), loaded_(true) {} // NOLINT

            /// @{
            /**
             * @brief Use default copy behaviour
             */
            BaseWrapper(const BaseWrapper& rhs) = default;
            BaseWrapper& operator=(const BaseWrapper& rhs) = default;
            /// @}

            /// @{
            /**
             * @brief Use default move behaviour
             */
            BaseWrapper(BaseWrapper&& rhs) = default;
            BaseWrapper& operator=(BaseWrapper&& rhs) = default;
            /// @}

            /**
             * @brief Getter function to retrieve pointer from wrapper object
             *
             * This function implements lazy loading and fetches the pointer from the TRef object in case the pointer is not
             * initialized yet
             *
             * @return Pointer to object
             */
            virtual T* get() const = 0;

            /**
             * @brief Function to construct TRef object for wrapped pointer for persistent storage
             */
            void store() { ref_ = get(); }

            ClassDef(BaseWrapper, 1); // NOLINT

        protected:
            /**
             * @brief Required virtual destructor
             */
            virtual ~BaseWrapper() = default;

            mutable T* ptr_{};           //! transient value
            mutable bool loaded_{false}; //! transient value
            TRef ref_{};
        };

        template <class T> class PointerWrapper : public BaseWrapper<T> {
        public:
            /**
             * @brief Using implicit and explicit constructors from base class
             */
            using BaseWrapper<T>::BaseWrapper;

            /**
             * @brief Required virtual destructor
             */
            ~PointerWrapper() override = default;

            /**
             * @brief Explicit copy constructor to avoid copying std::once_flag
             */
            PointerWrapper(const PointerWrapper& rhs) : BaseWrapper<T>(rhs){};

            /**
             * @brief Explicit copy assignment operator to avoid copying std::once_flag
             */
            PointerWrapper& operator=(const PointerWrapper& rhs) {
                BaseWrapper<T>::operator=(rhs);
                return *this;
            };

            /**
             * @brief Explicit move constructor to avoid copying std::once_flag
             */
            PointerWrapper(PointerWrapper&& rhs) noexcept : BaseWrapper<T>(std::move(rhs)){};

            /**
             * @brief Explicit move assignment to avoid copying std::once_flag
             */
            PointerWrapper& operator=(PointerWrapper&& rhs) noexcept {
                BaseWrapper<T>::operator=(std::move(rhs));
                return *this;
            };

            /**
             * @brief Implementation of base class lazy loading mechanism with thread-safe call_once
             * @return Pointer to object
             */
            T* get() const override {
                // Lazy loading of pointer from TRef
                std::call_once(load_flag_, [&]() {
                    if(!this->loaded_) {
                        this->ptr_ = static_cast<T*>(this->ref_.GetObject());
                        this->loaded_ = true;
                    }
                });
                return this->ptr_;
            };

            ClassDefOverride(PointerWrapper, 1); // NOLINT

        private:
            mutable std::once_flag load_flag_; //! transient value
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
