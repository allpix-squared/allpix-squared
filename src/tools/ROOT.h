/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// set of Geant4 extensions for the AllPix core

#ifndef ALLPIX_ROOT_H
#define ALLPIX_ROOT_H

#include <stdexcept>
#include <string>
#include <utility>

#include <Math/DisplacementVector2D.h>
#include <Math/DisplacementVector3D.h>
#include <Math/EulerAngles.h>
#include <Math/PositionVector2D.h>
#include <Math/PositionVector3D.h>
#include <TString.h>

#include "core/utils/string.h"
#include "core/utils/type.h"

namespace allpix {
    /** Extend to string and from string methods for ROOT */

    // 3D displacement vectors
    template <typename T>
    inline ROOT::Math::DisplacementVector3D<T> from_string_impl(std::string str,
                                                                type_tag<ROOT::Math::DisplacementVector3D<T>>) {
        std::vector<typename T::Scalar> vec_split = allpix::split<typename T::Scalar>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return ROOT::Math::DisplacementVector3D<T>(vec_split[0], vec_split[1], vec_split[2]);
    }
    template <typename T> inline std::string to_string_impl(const ROOT::Math::DisplacementVector3D<T>& vec, empty_tag) {
        std::string res;
        res += std::to_string(vec.x());
        res += " ";
        res += std::to_string(vec.y());
        res += " ";
        res += std::to_string(vec.z());
        return res;
    }

    // 2D displacement vectors
    template <typename T>
    inline ROOT::Math::DisplacementVector2D<T> from_string_impl(std::string str,
                                                                type_tag<ROOT::Math::DisplacementVector2D<T>>) {
        std::vector<typename T::Scalar> vec_split = allpix::split<typename T::Scalar>(std::move(str));
        if(vec_split.size() != 2) {
            throw std::invalid_argument("array should contain exactly two elements");
        }
        return ROOT::Math::DisplacementVector2D<T>(vec_split[0], vec_split[1]);
    }
    template <typename T> inline std::string to_string_impl(const ROOT::Math::DisplacementVector2D<T>& vec, empty_tag) {
        std::string res;
        res += std::to_string(vec.x());
        res += " ";
        res += std::to_string(vec.y());
        return res;
    }

    // 3D position vectors
    template <typename T>
    inline ROOT::Math::PositionVector3D<T> from_string_impl(std::string str, type_tag<ROOT::Math::PositionVector3D<T>>) {
        std::vector<typename T::Scalar> vec_split = allpix::split<typename T::Scalar>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return ROOT::Math::PositionVector3D<T>(vec_split[0], vec_split[1], vec_split[2]);
    }
    template <typename T> inline std::string to_string_impl(const ROOT::Math::PositionVector3D<T>& vec, empty_tag) {
        return to_string_impl(static_cast<ROOT::Math::DisplacementVector3D<T>>(vec), empty_tag());
    }

    // 2D position vectors
    template <typename T>
    inline ROOT::Math::PositionVector2D<T> from_string_impl(std::string str, type_tag<ROOT::Math::PositionVector2D<T>>) {
        std::vector<typename T::Scalar> vec_split = allpix::split<typename T::Scalar>(std::move(str));
        if(vec_split.size() != 2) {
            throw std::invalid_argument("array should contain exactly two elements");
        }
        return ROOT::Math::PositionVector2D<T>(vec_split[0], vec_split[1]);
    }
    template <typename T> inline std::string to_string_impl(const ROOT::Math::PositionVector2D<T>& vec, empty_tag) {
        return to_string_impl(static_cast<ROOT::Math::DisplacementVector2D<T>>(vec), empty_tag());
    }

    // Euler angles
    inline ROOT::Math::EulerAngles from_string_impl(std::string str, type_tag<ROOT::Math::EulerAngles>) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return ROOT::Math::EulerAngles(vec_split[0], vec_split[1], vec_split[2]);
    }
    inline std::string to_string_impl(const ROOT::Math::EulerAngles& vec, allpix::empty_tag) {
        std::string res;
        res += std::to_string(vec.Phi());
        res += " ";
        res += std::to_string(vec.Theta());
        res += " ";
        res += std::to_string(vec.Psi());
        return res;
    }

    // TString
    // FIXME: do we want this at all
    inline TString from_string_impl(std::string str, type_tag<TString>) {
        return TString(allpix::from_string<std::string>(std::move(str)).c_str());
    }
    inline std::string to_string_impl(const TString& str, empty_tag) { return std::string(str.Data()); }
} // namespace allpix

#endif /* ALLPIX_ROOT_H */
