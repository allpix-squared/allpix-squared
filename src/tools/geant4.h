/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// set of Geant4 extensions for the AllPix core

#ifndef ALLPIX_GEANT4_H
#define ALLPIX_GEANT4_H

#include <string>

#include "G4ThreeVector.hh"

#include "../core/utils/string.h"

namespace allpix{
    // extend to and from string methods for the configuration
    template <> inline G4ThreeVector from_string<G4ThreeVector> (std::string str){
        std::vector<double> vec_split = allpix::split<double>(str);
        if(vec_split.size() != 3) throw std::invalid_argument("array should contain exactly three elements");
        return G4ThreeVector(vec_split[0], vec_split[1], vec_split[2]);
    }
    template <> inline std::string to_string<G4ThreeVector> (G4ThreeVector vec){
        std::string res;
        for(int i = 0; i < 3; ++i){
            res += std::to_string(vec[i]);
            if(i != 2) res += " ";
        }
        return res;
    }
}

#endif
