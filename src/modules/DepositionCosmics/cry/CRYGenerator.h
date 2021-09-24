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

//
//  CRYGenerator
//
//  Created October, 2006
//
//  CRYGenerator is driver class to create and return primary
//  events
//

#ifndef CRYGenerator_h
#define CRYGenerator_h

#include <map>
#include <vector>
#include "CRYParticle.h"

#include "CRYPrimary.h"

class CRYUtils;
class CRYBinning;
class CRYPdf;
class CRYSetup;
class CRYWeightFunc;

class CRYGenerator {

public:
    CRYGenerator(CRYSetup* setup);

    // ways to generate an event
    // a single cosmic shower is returned
    // Ownership _IS_ returned
    std::vector<CRYParticle*>* genEvent();
    void genEvent(std::vector<CRYParticle*>* retList);

    // Time that has been simulated by this instance
    double timeSimulated() { return _primary->timeSimulated(); }

    // Pointer to the primary particle
    // Note that ownership is not transfered
    CRYParticle* primaryParticle() { return _primaryPart; }

    // which set of data tables was used
    // the particles returned will be in a smaller box
    // (as specified in the setup
    double boxSizeUsed() { return _boxSize; }

private:
    CRYPrimary* _primary;
    CRYUtils* _utils;
    CRYBinning *_primaryBinning, *_secondaryBinning;
    CRYPdf* _nParticlesPDF;
    CRYPdf* _particleFractionsPDF;
    std::map<int, CRYParticle::CRYId> _idDict;
    std::map<CRYParticle::CRYId, CRYPdf*> _kePdfs, _latPdfs;
    std::map<CRYParticle::CRYId, CRYPdf*> _timePdfs, _cosThetaPdfs, _chargePdfs;
    std::map<CRYParticle::CRYId, bool> _tallyList;
    double _boxSize;
    double _subboxSize;
    int _maxParticles, _minParticles;
    CRYSetup* _setup;
    CRYWeightFunc* _primaryWeighting;
    CRYParticle* _primaryPart;
};

#endif
