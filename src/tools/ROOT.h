/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// set of Geant4 extensions for the AllPix core

#ifndef ALLPIX_ROOT_H
#define ALLPIX_ROOT_H

#include <string>

#include <Math/EulerAngles.h>
#include <Math/Vector3D.h>
#include <TString.h>

#include "core/utils/string.h"

namespace allpix {
    // FIXME: improve this

    // extend to and from string methods for ROOT
    template <> inline ROOT::Math::XYZVector from_string<ROOT::Math::XYZVector>(std::string str) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return ROOT::Math::XYZVector(vec_split[0], vec_split[1], vec_split[2]);
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

    template <> inline ROOT::Math::EulerAngles from_string<ROOT::Math::EulerAngles>(std::string str) {
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

    // FIXME: do we actually want this at all
    template <> inline TString from_string<TString>(std::string str) {
        return TString(allpix::from_string<std::string>(std::move(str)).c_str());
    }
    template <> inline std::string to_string<const TString&>(const TString& str) { return std::string(str.Data()); }
}

#endif /* ALLPIX_ROOT_H */
