/**
 * @file
 * @brief Set of Geant4 utilities for framework integration
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_GEANT4_H
#define ALLPIX_GEANT4_H

#include <stdexcept>
#include <string>
#include <utility>

#include <G4ThreeVector.hh>
#include <G4TwoVector.hh>
#include <Math/DisplacementVector3D.h>
#include <Math/PositionVector3D.h>

#include "core/utils/text.h"
#include "core/utils/type.h"

/**
 * @brief Version of std::make_shared that does not delete the pointer
 *
 * This version is needed because some pointers are deleted by Geant4 internally, but they are stored as std::shared_ptr in
 * the framework.
 */
template <typename T, typename... Args> static std::shared_ptr<T> make_shared_no_delete(Args... args) {
    return std::shared_ptr<T>(new T(args...), [](T*) {});
}

namespace allpix {
    /**
     * @ingroup StringConversions
     * @brief Enable support to convert string directly to Geant4 3D vector while fetching configuration parameter
     */
    inline G4ThreeVector from_string_impl(std::string str, type_tag<G4ThreeVector>) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return {vec_split[0], vec_split[1], vec_split[2]};
    }
    /**
     * @ingroup StringConversions
     * @brief Enable support to convert Geant4 3D vector to string for storage in the configuration
     */
    inline std::string to_string_impl(const G4ThreeVector& vec, empty_tag) {
        std::string res;
        for(int i = 0; i < 3; ++i) {
            res += std::to_string(vec[i]);
            if(i != 2) {
                res += ",";
            }
        }
        return res;
    }

    /**
     * @ingroup StringConversions
     * @brief Enable support to convert string directly to Geant4 2D vector for fetching configuration parameter
     */
    inline G4TwoVector from_string_impl(std::string str, type_tag<G4TwoVector>) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 2) {
            throw std::invalid_argument("array should contain exactly two elements");
        }
        return {vec_split[0], vec_split[1]};
    }
    /**
     * @ingroup StringConversions
     * @brief Enable support to convert Geant4 2D vector to string for storage in the configuration
     */
    inline std::string to_string_impl(const G4TwoVector& vec, empty_tag) {
        std::string res;
        for(int i = 0; i < 2; ++i) {
            res += std::to_string(vec[i]);
            if(i != 1) {
                res += ",";
            }
        }
        return res;
    }

    /**
     * @brief Utility method to convert ROOT 3D vector to Geant4 3D vector
     */
    template <typename T> G4ThreeVector toG4Vector(const ROOT::Math::DisplacementVector3D<T>& vector) {
        return G4ThreeVector(vector.x(), vector.y(), vector.z());
    }
    /**
     * @brief Utility method to convert ROOT 2D vector to Geant4 2D vector
     */
    template <typename T> G4ThreeVector toG4Vector(const ROOT::Math::PositionVector3D<T>& vector) {
        return G4ThreeVector(vector.x(), vector.y(), vector.z());
    }
} // namespace allpix

#endif /* ALLPIX_GEANT4_H */
