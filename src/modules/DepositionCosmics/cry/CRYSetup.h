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
//  CRYPdf.h
//
//  Created October, 2006
//
//  CRYSetup keeps list of possible parameters and parses
//  their input from the config file
//

#ifndef CRYSETUP_H
#define CRYSETUP_H

#include <map>
#include <string>
#include "CRYUtils.h"

class CRYData;

class CRYSetup {

public:
    // list of all available parameters!
    enum CRYParms {
        parmMin,
        returnNeutrons = parmMin, // include neutrons in return list (0,1)
        returnProtons,            // include protons?
        returnGammas,             // include gammas?
        returnElectrons,          // include electrons?
        returnMuons,              // include muons?
        returnPions,              // include pions?
        returnKaons,              // include kaons?
        subboxLength,             // length of box to return particles in
        altitude,                 // Working altitude
        latitude,                 // Working latitude
        date,                     // Date (for solar cycle)
        nParticlesMin,            // Minimum # of particles to return
        nParticlesMax,            // Maximum # of particles to return
        xoffset,
        yoffset,
        zoffset,
        parmMax = zoffset
    };

    // Nominal constructor
    // config file format is
    //  <key> <value>
    // separated by whitespace
    // Keys are defined in constructor of CRYSetup
    CRYSetup(std::string configData, std::string dataDir = "./");

    // Nothing to delete
    ~CRYSetup() {}

    // Get value of parameter
    double param(CRYSetup::CRYParms parm) { return _parms[parm]; }
    void setParam(CRYSetup::CRYParms parm, double value) { _parms[parm] = value; }

    void setRandomFunction(double (*newFunc)(void)) { _utils->setRandomFunction(newFunc); }

    CRYData* getData(int altit = 0) { return _data[altit]; }
    CRYUtils* getUtils() { return _utils; }

private:
    // Map of enums to parameter values
    std::map<CRYSetup::CRYParms, double> _parms;
    // Map of enums to parameter names
    std::map<CRYSetup::CRYParms, std::string> _parmNames;

    CRYUtils* _utils;
    std::map<int, CRYData*> _data;

    double parseDate(std::string dt); //....convert date string to decimal year
    bool isLeapYear(int yr);          // Returns true if yr is a leap year, false if not
};

#endif
