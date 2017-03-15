/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// set of Geant4 extensions for the AllPix core

#ifndef ALLPIX_GEANT4_H
#define ALLPIX_GEANT4_H

#include <string>

#include <TVector3.h>
#include "G4ThreeVector.hh"
#include "G4TwoVector.hh"

#include "core/utils/string.h"

namespace allpix {
    // FIXME: improve this

    // extend to and from string methods for Geant4
    template <> inline G4ThreeVector from_string<G4ThreeVector>(std::string str) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 3) {
            throw std::invalid_argument("array should contain exactly three elements");
        }
        return G4ThreeVector(vec_split[0], vec_split[1], vec_split[2]);
    }
    template <> inline std::string to_string<G4ThreeVector>(G4ThreeVector vec) {
        std::string res;
        for(int i = 0; i < 3; ++i) {
            res += std::to_string(vec[i]);
            if(i != 2) {
                res += " ";
            }
        }
        return res;
    }

    template <> inline G4TwoVector from_string<G4TwoVector>(std::string str) {
        std::vector<double> vec_split = allpix::split<double>(std::move(str));
        if(vec_split.size() != 2) {
            throw std::invalid_argument("array should contain exactly two elements");
        }
        return G4TwoVector(vec_split[0], vec_split[1]);
    }
    template <> inline std::string to_string<G4TwoVector>(G4TwoVector vec) {
        std::string res;
        for(int i = 0; i < 2; ++i) {
            res += std::to_string(vec[i]);
            if(i != 1) {
                res += " ";
            }
        }
        return res;
    }

    // convert G4 vector to ROOT vector
    inline TVector3 toROOTVector(const G4ThreeVector& vector) { return TVector3(vector.x(), vector.y(), vector.z()); }
}

#endif
