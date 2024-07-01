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

#include "CRYUtils.h"
#include <iostream>

CRYUtils::CRYUtils() {
    rngdptr = nullptr;
    setRandomFunction(tmpRandom);
}

std::string CRYUtils::removeTrailingSpaces(std::string input) {

    std::string t(" \t\r\n");
    std::string d(input);
    std::string::size_type i(d.find_last_not_of(t));
    if(i == std::string::npos)
        return "";
    else
        return d.erase(d.find_last_not_of(t) + 1);
}

// until we get one from the transport coders
double CRYUtils::randomFlat(double min, double max) { return min + (max - min) * rngdptr(); }

double CRYUtils::tmpRandom() {
    static unsigned long int next = 1;

    next = next * 1103515245 + 123345;
    unsigned temp = static_cast<unsigned>(next / 65536) % 32768;

    return (temp + 1.0) / 32769.0;
}

std::string CRYUtils::partName(CRYParticle::CRYId id) {
    if(id == CRYParticle::Neutron)
        return "neutron";
    if(id == CRYParticle::Proton)
        return "proton";
    if(id == CRYParticle::Pion)
        return "pion";
    if(id == CRYParticle::Kaon)
        return "kaon";
    if(id == CRYParticle::Muon)
        return "muon";
    if(id == CRYParticle::Electron)
        return "electron";
    if(id == CRYParticle::Gamma)
        return "gamma";
    return "???";
}
