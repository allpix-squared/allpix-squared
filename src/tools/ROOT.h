/**
 * @file
 * @brief Set of ROOT utilities for framework integration
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
#include <TString.h>

#include <ROOT/TThreadedObject.hxx>
#include <TH1.h>

#include "core/utils/text.h"
#include "core/utils/type.h"

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
     * @brief A thin wrapper over ROOT::TThreadedObject for histograms
     *
     * Enables filling histograms in parallel and makes sure an empty instance will exist if not filled.
     */
    template <typename T, typename std::enable_if<std::is_base_of<TH1, T>::value>::type* = nullptr>
    class ThreadedHistogram : public ROOT::TThreadedObject<T> {
    public:
        template <class... ARGS> ThreadedHistogram(ARGS&&... args) : ROOT::TThreadedObject<T>(std::forward<ARGS>(args)...) {
            this->Get();
        }

        /**
         * @brief An easy way to fill a histogram
         */
        template <class... ARGS> Int_t Fill(ARGS&&... args) { return this->Get()->Fill(std::forward<ARGS>(args)...); }

        /**
         * @brief An easy way to write a histogram
         */
        void Write() { this->Merge()->Write(); }
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
} // namespace allpix

#endif /* ALLPIX_ROOT_H */
