/*

Copyright (c) 2007-2012, The Regents of the University of California.
Produced at the Lawrence Livermore National Laboratory
UCRL-CODE-227323.
All rights reserved.

For details, see http://nuclear.llnl.gov/simulations
Please also read this http://nuclear.llnl.gov/simulations/additional_bsd.html

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
notice, this list of conditions and the disclaimer below.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the disclaimer (as noted below) in
the documentation and/or other materials provided with the
distribution.

3. Neither the name of the UC/LLNL nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OF
THE UNIVERSITY OF CALIFORNIA, THE U.S. DEPARTMENT OF ENERGY OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CRYFunctionDict.h"
#include <assert.h>
#include <iostream>
#include <sstream>
#include <string.h> // For Ubuntu Linux
#include <string>
#include "CRYAbsFunction.h"
#include "CRYCosLatitudeFunction.h"
#include "CRYPrimarySpectrumFunction.h"
#include "CRYUtils.h"

CRYFunctionDict::CRYFunctionDict() {

    _knownFunctions[CRYFunctionDict::PrimarySpectrum1] = "PrimarySpectrum1";
    _knownFunctions[CRYFunctionDict::cosLatitude] = "cosLatitude";
}

CRYAbsFunction* CRYFunctionDict::function(std::string data) {

    functype type = CRYFunctionDict::UNKNOWN;

    std::istringstream iss(data);
    std::string lhs;
    std::string rhs;

    std::getline(iss, lhs, '=');
    std::getline(iss, rhs, '=');

    // then lhs has the function name..
    std::istringstream iss2(lhs);
    std::string key = "";
    std::string name = "";
    ;
    int c = 0;
    while(!iss2.eof()) {
        std::getline(iss2, key, ' ');
        if(0 != strcmp(key.c_str(), " ") && 0 != strcmp(key.c_str(), "")) {
            c++;
            if(c != 1)
                name.append(key);
        }
    }
    // but this name must have a :: that indicates the binning to use
    std::string funcName;
    std::string funcType;

    std::string::size_type colLoc = name.find(":");
    if(colLoc == std::string::npos || name[colLoc + 1] != ':') {
        std::cerr << "CRY::CRYFunctionDict: Function must specify type. Data was:\n";
        std::cerr << data << std::endl;
        assert(0);
    }

    std::istringstream issName(name);
    std::getline(issName, funcName, ':');
    // dummy get...
    std::getline(issName, funcType, ':');
    std::getline(issName, funcType, ':');

    funcType = CRYUtils::removeTrailingSpaces(funcType);

    std::map<functype, std::string>::iterator iterKF;
    for(iterKF = _knownFunctions.begin(); iterKF != _knownFunctions.end(); iterKF++) {
        if(iterKF->second == funcType)
            type = iterKF->first;
    }
    if(type == CRYFunctionDict::UNKNOWN) {
        std::cerr << "CRY::CRYFunctionDict: Unknown function type " << funcType << " Data was:\n";
        std::cerr << data << std::endl;
        assert(0);
    }

    if(type == CRYFunctionDict::PrimarySpectrum1)
        return new CRYPrimarySpectrumFunction(funcName, rhs);
    if(type == CRYFunctionDict::cosLatitude)
        return new CRYCosLatitudeFunction(funcName, rhs);

    return 0;
}
