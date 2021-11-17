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
//  CRYPrimary
//
//  Created October, 2006
//
//  CRYPrimary creates primary protons given specified spectra
//  parameters and optionally an input weight function
//

#ifndef CRYPrimary_h
#define CRYPrimary_h

#include <vector>

class CRYUtils;
class CRYData;
class CRYParticle;
class CRYAbsFunction;
class CRYUtils;
class CRYParameter;
class CRYWeightFunc;
class CRYBinning;
class CRYPdf;

class CRYPrimary {

public:
    // data is the data table
    // date is in years to approximate the solar cycle
    // latitude is in degrees!
    CRYPrimary(CRYUtils* utils, CRYData* data, double date = 2007, double latitude = 0);

    ~CRYPrimary() { ; }

    // Given the parameters specified in the constnructor
    // and by setWeightFunc (if needed) - a new primary proton
    // is created and returned for further analysis
    CRYParticle* getPrimary();

    // Given the function parameters (but NOT the weights)
    // determine the rate in the specified binning
    std::vector<double> partialRates(const std::vector<double>* bins) const;
    std::vector<double> partialRates(const CRYBinning* bins = nullptr) const;

    // either add or recompute weighting from existing function
    void setWeightFunc(double area, CRYWeightFunc* wf = nullptr);

    // The time elapsed during the simulation of primaries
    double timeSimulated() { return _dt; }

    // Sum of unweighted partial rates
    double totalRate();

private:
    // primary spectrum is a weighted sum of solar min and solar
    // max parameters
    CRYAbsFunction* _solarMin;
    CRYAbsFunction* _solarMax;
    CRYParameter* _solarCycleStart;
    CRYParameter* _solarCycleLength;
    double _cycle;

    CRYUtils* _utils; // random numbers

    // Energy boundaries
    double _minEnergy, _maxEnergy;
    CRYBinning* _binning;

    // Optional weighting function
    CRYWeightFunc* _wf;

    // Computed lifetime
    double _lifeTime;

    // Time simulated
    double _dt;

    // Store the maximum of PDF for comparison with randoms
    double _maxPDF;

    // Compute (using weighting function) the maximum pdf value
    void calcMaxPDF();

    CRYPdf* _cachedPdf;
};

#endif
