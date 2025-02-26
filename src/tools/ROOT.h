/**
 * @file
 * @brief Set of ROOT utilities for framework integration
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_ROOT_H
#define ALLPIX_ROOT_H

#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/DisplacementVector2D.h>
#include <Math/DisplacementVector3D.h>
#include <Math/EulerAngles.h>
#include <Math/PositionVector2D.h>
#include <Math/PositionVector3D.h>
#include <RVersion.h>
#include <TProcessID.h>
#include <TString.h>

#include <ROOT/TThreadedObject.hxx>
#include <TH1.h>

#include "core/module/ThreadPool.hpp"
#include "core/utils/text.h"
#include "core/utils/type.h"

#include "core/utils/log.h"

namespace allpix {
    /**
     * @ingroup StringConversions
     * @brief Enable support to convert string directly to ROOT 3D displacement vector while fetching configuration parameter
     */
    template <typename T>
    inline ROOT::Math::DisplacementVector3D<T> from_string_impl(std::string str,
                                                                type_tag<ROOT::Math::DisplacementVector3D<T>>) {
        std::vector<typename T::Scalar> vec_split = allpix::split<typename T::Scalar>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return ROOT::Math::DisplacementVector3D<T>(vec_split[0], vec_split[1], vec_split[2]);
    }
    /**
     * @ingroup StringConversions
     * @brief Enable support to convert ROOT 3D displacement vector to string for storage in the configuration
     */
    template <typename T> inline std::string to_string_impl(const ROOT::Math::DisplacementVector3D<T>& vec, empty_tag) {
        std::string res;
        res += std::to_string(vec.x());
        res += ",";
        res += std::to_string(vec.y());
        res += ",";
        res += std::to_string(vec.z());
        return res;
    }

    /**
     * @ingroup StringConversions
     * @brief Enable support to convert string directly to ROOT 2D displacement vector while fetching configuration parameter
     */
    template <typename T>
    inline ROOT::Math::DisplacementVector2D<T> from_string_impl(std::string str,
                                                                type_tag<ROOT::Math::DisplacementVector2D<T>>) {
        std::vector<typename T::Scalar> vec_split = allpix::split<typename T::Scalar>(std::move(str));
        if(vec_split.size() != 2) {
            throw std::invalid_argument("array should contain exactly two elements");
        }
        return ROOT::Math::DisplacementVector2D<T>(vec_split[0], vec_split[1]);
    }
    /**
     * @ingroup StringConversions
     * @brief Enable support to convert ROOT 2D displacement vector to string for storage in the configuration
     */
    template <typename T> inline std::string to_string_impl(const ROOT::Math::DisplacementVector2D<T>& vec, empty_tag) {
        std::string res;
        res += std::to_string(vec.x());
        res += ",";
        res += std::to_string(vec.y());
        return res;
    }

    /**
     * @ingroup StringConversions
     * @brief Enable support to convert string directly to ROOT 3D position vector while fetching configuration parameter
     */
    template <typename T>
    inline ROOT::Math::PositionVector3D<T> from_string_impl(std::string str, type_tag<ROOT::Math::PositionVector3D<T>>) {
        std::vector<typename T::Scalar> vec_split = allpix::split<typename T::Scalar>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return ROOT::Math::PositionVector3D<T>(vec_split[0], vec_split[1], vec_split[2]);
    }
    /**
     * @ingroup StringConversions
     * @brief Enable support to convert ROOT 3D position vector to string for storage in the configuration
     */
    template <typename T> inline std::string to_string_impl(const ROOT::Math::PositionVector3D<T>& vec, empty_tag) {
        return to_string_impl(static_cast<ROOT::Math::DisplacementVector3D<T>>(vec), empty_tag());
    }

    /**
     * @ingroup StringConversions
     * @brief Enable support to convert string directly to ROOT 2D position vector while fetching configuration parameter
     */
    template <typename T>
    inline ROOT::Math::PositionVector2D<T> from_string_impl(std::string str, type_tag<ROOT::Math::PositionVector2D<T>>) {
        std::vector<typename T::Scalar> vec_split = allpix::split<typename T::Scalar>(std::move(str));
        if(vec_split.size() != 2) {
            throw std::invalid_argument("array should contain exactly two elements");
        }
        return ROOT::Math::PositionVector2D<T>(vec_split[0], vec_split[1]);
    }
    /**
     * @ingroup StringConversions
     * @brief Enable support to convert ROOT 2D position vector to string for storage in the configuration
     */
    template <typename T> inline std::string to_string_impl(const ROOT::Math::PositionVector2D<T>& vec, empty_tag) {
        return to_string_impl(static_cast<ROOT::Math::DisplacementVector2D<T>>(vec), empty_tag());
    }

    /**
     * @brief Overload output stream operator to display ROOT 3D displacement vector
     */
    template <typename T, typename U>
    inline std::ostream& operator<<(std::ostream& os, const ROOT::Math::DisplacementVector3D<T, U>& vec) {
        return os << "(" << vec.x() << "," << vec.y() << "," << vec.z() << ")";
    }
    /**
     * @brief Overload output stream operator to display ROOT 2D displacement vector
     */
    template <typename T, typename U>
    inline std::ostream& operator<<(std::ostream& os, const ROOT::Math::DisplacementVector2D<T, U>& vec) {
        return os << "(" << vec.x() << "," << vec.y() << ")";
    }
    /**
     * @brief Overload output stream operator to display ROOT 3D position vector
     */
    template <typename T, typename U>
    inline std::ostream& operator<<(std::ostream& os, const ROOT::Math::PositionVector3D<T, U>& vec) {
        return os << "(" << vec.x() << "," << vec.y() << "," << vec.z() << ")";
    }
    /**
     * @brief Overload output stream operator to display ROOT 2D position vector
     */
    template <typename T, typename U>
    inline std::ostream& operator<<(std::ostream& os, const ROOT::Math::PositionVector2D<T, U>& vec) {
        return os << "(" << vec.x() << "," << vec.y() << ")";
    }

    /**
     * @brief A re-implementation of ROOT::TThreadedObject
     *
     * This class is a re-implementation of TThreadedObject for histograms and profiles, providing better scalability and an
     * additional thin wrapper to commonly used histogram functions such as Fill() or SetBinContent(). Furthermore, it also
     * does not depend on ROOT implementation changes that have happened to the original class between minor ROOT versions.
     * This class scales to an arbitrary number of thread, irrespective of the underlying ROOT version.
     *
     * Enables filling histograms in parallel and makes sure an empty instance will exist if not filled.
     */
    template <typename T, typename std::enable_if<std::is_base_of<TH1, T>::value>::type* = nullptr> class ThreadedHistogram {
    public:
        template <class... ARGS> explicit ThreadedHistogram(ARGS&&... args) { this->init(std::forward<ARGS>(args)...); }

        /**
         * @brief An easy way to fill a histogram
         */
        template <class... ARGS> Int_t Fill(ARGS&&... args) { // NOLINT
            return this->Get()->Fill(std::forward<ARGS>(args)...);
        }

        /**
         * @brief An easy way to set bin contents
         */
        template <class... ARGS> void SetBinContent(ARGS&&... args) { // NOLINT
            this->Get()->SetBinContent(std::forward<ARGS>(args)...);
        }

        /**
         * @brief An easy way to write a histogram
         */
        void Write() { this->Merge()->Write(); } // NOLINT

        /**
         * @brief Get the thread local instance of the histogram
         *
         * Based on get in https://root.cern/doc/master/classROOT_1_1TThreadedObject.html, optimized for faster retrieval.
         */
        std::shared_ptr<T> Get() { // NOLINT
            auto idx = ThreadPool::threadNum();
            auto& object = objects_[idx];
            if(!object) {
                object.reset(ROOT::Internal::TThreadedObjectUtils::Cloner<T>::Clone(model_.get(), directories_[idx]));
            }
            return object;
        }

        /**
         * @brief Merge the threaded histograms into final object
         *
         * Based on merging in https://root.cern/doc/master/classROOT_1_1TThreadedObject.html.
         */
        std::shared_ptr<T> Merge() { // NOLINT
            ROOT::TThreadedObjectUtils::MergeFunctionType<T> mergeFunction = ROOT::TThreadedObjectUtils::MergeTObjects<T>;
            if(is_merged_) {
                return objects_[0];
            }
            mergeFunction(objects_[0], objects_);
            is_merged_ = true;
            return objects_[0];
        }

    private:
        /**
         * @brief Initialize the threaded histogram
         *
         * Based on initialization in https://root.cern/doc/master/classROOT_1_1TThreadedObject.html, modified to
         * initialize based on number of preregistered threads.
         */
        template <class... ARGS> void init(ARGS&&... args) {
            const auto num_slots = ThreadPool::threadCount();
            objects_.resize(num_slots);

#if ROOT_VERSION_CODE < ROOT_VERSION(6, 22, 0)
            directories_ = ROOT::Internal::TThreadedObjectUtils::DirCreator<T>::Create(num_slots);
#else
            // create at least one directory (we need it for the model), plus others as needed by the size of objects
            directories_.emplace_back(ROOT::Internal::TThreadedObjectUtils::DirCreator<T>::Create());
            for(auto i = 1u; i < num_slots; ++i) {
                directories_.emplace_back(ROOT::Internal::TThreadedObjectUtils::DirCreator<T>::Create());
            }
#endif

            TDirectory::TContext ctxt(directories_[0]);
            model_.reset(ROOT::Internal::TThreadedObjectUtils::Detacher<T>::Detach(new T(std::forward<ARGS>(args)...)));

            // initialize at least the base object
            objects_[0].reset(ROOT::Internal::TThreadedObjectUtils::Cloner<T>::Clone(model_.get(), directories_[0]));
        }

        std::unique_ptr<T> model_;
        std::vector<std::shared_ptr<T>> objects_;
        std::vector<TDirectory*> directories_;
        bool is_merged_{false};
    };

    /**
     * @brief Helper method to instantiate new objects of the type ThreadedHistogram
     *
     * @param args Arguments passed to histogram class
     * @return Unique pointer to newly created object
     */
    template <typename T, class... ARGS> std::unique_ptr<ThreadedHistogram<T>> CreateHistogram(ARGS&&... args) {
        return std::make_unique<ThreadedHistogram<T>>(std::forward<ARGS>(args)...);
    }

    template <class T> using Histogram = std::unique_ptr<ThreadedHistogram<T>>;

    /**
     * @brief Lock for TProcessID simultaneous action
     */
    inline std::unique_lock<std::mutex> root_process_lock() {
        static std::mutex process_id_mutex;
        std::unique_lock<std::mutex> lock(process_id_mutex);

        auto* pids = TProcessID::GetPIDs();
        for(int i = 0; i < pids->GetEntries(); ++i) {
            auto* pid_ptr = static_cast<TProcessID*>((*pids)[i]);
            pid_ptr->Clear();
        }

        return lock;
    }
} // namespace allpix

#endif /* ALLPIX_ROOT_H */
