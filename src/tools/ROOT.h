/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// set of Geant4 extensions for the AllPix core

#ifndef ALLPIX_ROOT_H
#define ALLPIX_ROOT_H

#include <string>

#include <Math/EulerAngles.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>
#include <TString.h>

#include "core/utils/string.h"

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
    template <> inline std::string to_string<const ROOT::Math::XYZVector&>(const ROOT::Math::XYZVector& vec) {
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
    template <> inline std::string to_string<const ROOT::Math::XYVector&>(const ROOT::Math::XYVector& vec) {
        std::string res;
        res += std::to_string(vec.x());
        res += " ";
        res += std::to_string(vec.y());
        return res;
    }

    // Euler angles
    inline ROOT::Math::EulerAngles from_string_impl(std::string str, type_tag<ROOT::Math::EulerAngles>) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return ROOT::Math::EulerAngles(vec_split[0], vec_split[1], vec_split[2]);
    }
    template <> inline std::string to_string<const ROOT::Math::EulerAngles&>(const ROOT::Math::EulerAngles& vec) {
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
    template <> inline std::string to_string<const TString&>(const TString& str) { return std::string(str.Data()); }
}

#endif /* ALLPIX_ROOT_H */
