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

#include <TAxis.h>
#include <TH1.h>
#include <TObject.h>

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
                                                                type_tag<ROOT::Math::DisplacementVector3D<T>> /*unused*/) {
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
    template <typename T>
    inline std::string to_string_impl(const ROOT::Math::DisplacementVector3D<T>& vec, empty_tag /*unused*/) {
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
                                                                type_tag<ROOT::Math::DisplacementVector2D<T>> /*unused*/) {
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
    template <typename T>
    inline std::string to_string_impl(const ROOT::Math::DisplacementVector2D<T>& vec, empty_tag /*unused*/) {
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
    inline ROOT::Math::PositionVector3D<T> from_string_impl(std::string str,
                                                            type_tag<ROOT::Math::PositionVector3D<T>> /*unused*/) {
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
    template <typename T>
    inline std::string to_string_impl(const ROOT::Math::PositionVector3D<T>& vec, empty_tag /*unused*/) {
        return to_string_impl(static_cast<ROOT::Math::DisplacementVector3D<T>>(vec), empty_tag());
    }

    /**
     * @ingroup StringConversions
     * @brief Enable support to convert string directly to ROOT 2D position vector while fetching configuration parameter
     */
    template <typename T>
    inline ROOT::Math::PositionVector2D<T> from_string_impl(std::string str,
                                                            type_tag<ROOT::Math::PositionVector2D<T>> /*unused*/) {
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
    template <typename T>
    inline std::string to_string_impl(const ROOT::Math::PositionVector2D<T>& vec, empty_tag /*unused*/) {
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
     * @brief Lock for TProcessID simultaneous action
     */
    inline std::unique_lock<std::mutex> root_process_lock() {
        static std::mutex process_id_mutex;
        std::unique_lock<std::mutex> lock(process_id_mutex);

        auto* pids = TProcessID::GetPIDs();
        for(int i = 0; i < pids->GetEntries(); ++i) {
            auto* pid_ptr = dynamic_cast<TProcessID*>((*pids)[i]);
            pid_ptr->Clear();
        }

        return lock;
    }
} // namespace allpix

#endif /* ALLPIX_ROOT_H */
