/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// set of Geant4 extensions for the AllPix core

#ifndef ALLPIX_GEANT4_H
#define ALLPIX_GEANT4_H

#include <stdexcept>
#include <string>
#include <utility>

#include <G4ThreeVector.hh>
#include <G4TwoVector.hh>
#include <Math/Vector3D.h>

#include "core/utils/string.h"
#include "core/utils/type.h"

namespace allpix {
    /** Extend to string and from string methods for Geant4 */

    // 3D vector
    inline G4ThreeVector from_string_impl(std::string str, type_tag<G4ThreeVector>) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return G4ThreeVector(vec_split[0], vec_split[1], vec_split[2]);
    }
    inline std::string to_string_impl(const G4ThreeVector& vec, empty_tag) {
        std::string res;
        for(int i = 0; i < 3; ++i) {
            res += std::to_string(vec[i]);
            if(i != 2) {
                res += " ";
            }
        }
        return res;
    }

    // 2D vector
    inline G4TwoVector from_string_impl(std::string str, type_tag<G4TwoVector>) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 2) {
            throw std::invalid_argument("array should contain exactly two elements");
        }
        return G4TwoVector(vec_split[0], vec_split[1]);
    }
    inline std::string to_string_impl(const G4TwoVector& vec, empty_tag) {
        std::string res;
        for(int i = 0; i < 2; ++i) {
            res += std::to_string(vec[i]);
            if(i != 1) {
                res += " ";
            }
        }
        return res;
    }

    // FIXME: do we want this at all
    // convert G4 vector to ROOT vector
    template <typename T> G4ThreeVector toG4Vector(const ROOT::Math::DisplacementVector3D<T>& vector) {
        return G4ThreeVector(vector.x(), vector.y(), vector.z());
    }
    // convert G4 vector to ROOT vector
    inline ROOT::Math::XYZVector toROOTVector(const G4ThreeVector& vector) {
        return ROOT::Math::XYZVector(vector.x(), vector.y(), vector.z());
    }
} // namespace allpix

#endif
