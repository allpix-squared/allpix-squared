/**
 * @file
 * @brief Definition of Object base class
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

/**
 * @defgroup Objects Objects
 * @brief Collection of objects passed around between modules
 */

#ifndef ALLPIX_OBJECT_H
#define ALLPIX_OBJECT_H

#include <atomic>
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
        ClassDefOverride(Object, 4); // NOLINT

        /**
         * @brief Resolve all the history to standard pointers
         */
        virtual void loadHistory() = 0;
        /**
         * @brief Petrify all the pointers to prepare for persistent storage
         */
        virtual void petrifyHistory() = 0;

        void markForStorage() {
            // Using bit 14 of the TObject bit field, unused by ROOT:
            this->SetBit(1ull << 14);
        }

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
            std::cout << '\n';
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
            explicit BaseWrapper(const T* obj) : ptr_(const_cast<T*>(obj)) {} // NOLINT

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
            BaseWrapper(BaseWrapper&& rhs) = default;            // NOLINT
            BaseWrapper& operator=(BaseWrapper&& rhs) = default; // NOLINT
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
             *
             * @note A TRef is only constructed if the object the wrapped pointer is referring to has been marked for storage
             */
            void store() {
                if(markedForStorage()) {
                    ref_ = get();
                }
            }

            ClassDef(BaseWrapper, 1); // NOLINT

        protected:
            /**
             * @brief Required virtual destructor
             */
            virtual ~BaseWrapper() = default;

            /**
             * @brief Helper to determine whether the pointed object will be stored
             * @return True if object will be stored, false otherwise
             */
            bool markedForStorage() const { return get() == nullptr ? false : get()->TestBit(1ull << 14); }

            mutable T* ptr_{}; //! transient value
            TRef ref_;
        };

        template <class T> class PointerWrapper : public BaseWrapper<T> {
        public:
            /**
             * @brief Required default constructor
             */
            PointerWrapper() = default;

            /**
             * @brief Constructor with object pointer to be wrapped
             * @param obj Pointer to object
             */
            explicit PointerWrapper(const T* obj) : BaseWrapper<T>(obj), loaded_(true) {} // NOLINT

            /**
             * @brief Required virtual destructor
             */
            ~PointerWrapper() override = default;

            /**
             * @brief Explicit copy constructor to avoid copying std::once_flag
             */
            PointerWrapper(const PointerWrapper& rhs) : BaseWrapper<T>(rhs), loaded_(rhs.loaded_.load()) {};

            /**
             * @brief Explicit copy assignment operator to avoid copying std::once_flag
             */
            PointerWrapper& operator=(const PointerWrapper& rhs) {
                BaseWrapper<T>::operator=(rhs);
                loaded_ = rhs.loaded_.load();
                return *this;
            };

            /**
             * @brief Explicit move constructor to avoid copying std::once_flag
             */
            PointerWrapper(PointerWrapper&& rhs) noexcept : BaseWrapper<T>(std::move(rhs)), loaded_(rhs.loaded_.load()) {};

            /**
             * @brief Explicit move assignment to avoid copying std::once_flag
             */
            PointerWrapper& operator=(PointerWrapper&& rhs) noexcept {
                BaseWrapper<T>::operator=(std::move(rhs));
                loaded_ = rhs.loaded_.load();
                return *this;
            };

            /**
             * @brief Implementation of base class lazy loading mechanism with thread-safe call_once
             * @return Pointer to object
             */
            T* get() const override {
                // Lazy loading of pointer from TRef
                if(!this->loaded_) {
                    std::call_once(load_flag_, [&]() {
                        this->ptr_ = static_cast<T*>(this->ref_.GetObject());
                        this->loaded_ = true;
                    });
                }
                return this->ptr_;
            };

            ClassDefOverride(PointerWrapper, 1); // NOLINT

        private:
            mutable std::once_flag load_flag_;       //! transient value
            mutable std::atomic_bool loaded_{false}; //! transient value
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
