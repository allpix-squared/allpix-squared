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
// This class calculates the primary cosmic-ray flux
// (only protons for now) with corrections for latitude
// and time within sun spot variation cycle.
//
// getPrimary =  function will return a single primary cosmic ray
// sampled from the flux distribution.
//

#include "core/utils/log.h"

#include "CRYAbsFunction.h"
#include "CRYBinning.h"
#include "CRYData.h"
#include "CRYParameter.h"
#include "CRYParticle.h"
#include "CRYPdf.h"
#include "CRYPrimary.h"
#include "CRYUtils.h"
#include "CRYWeightFunc.h"

#include <assert.h>
#include <iostream>
#include <math.h>

CRYPrimary::CRYPrimary(CRYUtils* utils, CRYData* data, double date, double latitude) {

    _dt = 0.;
    _utils = utils;
    _solarMin = data->getFunction("primarySpectrumSolarMin");
    _solarMax = data->getFunction("primarySpectrumSolarMax");

    _solarCycleStart = data->getParameter("solarMinDate");
    _solarCycleLength = data->getParameter("solarCycleLength");

    if(_solarMin == nullptr) {
        LOG(ERROR) << "CRY::CRYPrimary: Missing solar minimum function" << std::endl;
        assert(0);
    }
    if(_solarMax == nullptr) {
        LOG(ERROR) << "CRY::CRYPrimary: Missing solar maximum function" << std::endl;
        assert(0);
    }

    // Location in solar cycle (0=min, 1=max)
    _cycle = fabs(sin(M_PI * (date - _solarCycleStart->param()) / _solarCycleLength->param()));

    _binning = data->getBinning("primaryBins");
    if(_binning == nullptr) {
        LOG(ERROR) << "CRY::CRYPrimary: Missing primaryBins bining\n";
        assert(0);
    }

    _minEnergy = _binning->min();
    _maxEnergy = _binning->max();

    CRYAbsFunction* cutoffMaker = data->getFunction("bfieldCorr");
    _minEnergy = std::max(_minEnergy, cutoffMaker->value(latitude));

    _cachedPdf = nullptr;
    setWeightFunc(1.0, nullptr); //....precompute the primary flux PDF
}

//
// return a primary cosmic-ray particle (only protons for now)
// sampled from the cosmic-ray flux PDF that was precomputed
//
CRYParticle* CRYPrimary::getPrimary() {

    double kine = 0.;

    kine = _cachedPdf->draw(_utils, 0);
    _dt += -_lifeTime * log(_utils->randomFlat());
    return new CRYParticle(CRYParticle::Proton, 0, kine);
}

std::vector<double> CRYPrimary::partialRates(const std::vector<double>* bins) const {

    std::vector<double> retVal;

    for(unsigned int i = 0; i < bins->size() - 1; i++) {
        double retValB = 0.;
        double lowB = (*bins)[i];
        double highB = (*bins)[i + 1];

        for(int j = 0; j < 1000; j++) {
            double kine = lowB + 0.001 * j * (highB - lowB);
            if(kine < _minEnergy)
                continue;
            retValB += (1.0 - _cycle) * _solarMin->value(kine) + _cycle * _solarMax->value(kine);
        }
        retValB *= 0.001 * (highB - lowB);
        retVal.push_back(retValB);
    }

    return retVal;
}

std::vector<double> CRYPrimary::partialRates(const CRYBinning* bins) const {

    const CRYBinning* binning = nullptr;
    if(bins != nullptr)
        binning = bins;
    else
        binning = _binning;

    const std::vector<double>* binVect = binning->bins();
    return partialRates(binVect);
}

double CRYPrimary::totalRate() {

    double minl10 = log10(_minEnergy);
    double maxl10 = log10(_maxEnergy);

    double retVal = 0.;
    for(int i = 0; i < 10000; i++) {
        double kine = pow(10.0, minl10 + (i + 0.5) * 0.0001 * (maxl10 - minl10));
        double retValt = 0.;
        retValt += (1.0 - _cycle) * _solarMin->value(kine) + _cycle * _solarMax->value(kine);
        retVal += retValt * (pow(10.0, minl10 + (i + 1.0) * 0.0001 * (maxl10 - minl10)) -
                             pow(10.0, minl10 + (i) * 0.0001 * (maxl10 - minl10)));
    }
    return retVal;
}

void CRYPrimary::setWeightFunc(double area, CRYWeightFunc* wf) {

    _wf = wf;

    if(_wf == nullptr) {
        _lifeTime = 1.0 / totalRate();
        calcMaxPDF();
        return;
    }

    const CRYBinning* binning = _wf->bins();
    const std::vector<double>* bins = binning->bins();
    std::vector<double> primaryPartialRates = partialRates(bins);

    // Need to create a PDF out of others to determine the
    // fraction of time in each primary bin that there are >0 particles
    double primaryRate = 0.;
    std::vector<double> fractionWithParticles;

    for(unsigned int i = 0; i < primaryPartialRates.size(); i++)
        primaryRate += primaryPartialRates[i] * _wf->weightBin(i);

    // lifetime between events with at least one particle
    _lifeTime = 1.0 / (primaryRate * M_PI * area);

    //  std::cout << "CRY::CRYPrimary: lifetime: " << _lifeTime << std::endl;

    calcMaxPDF();
}

void CRYPrimary::calcMaxPDF() {

    delete _cachedPdf;

    double minl10 = log10(_minEnergy);
    double maxl10 = log10(_maxEnergy);

    std::vector<double> pdfValues;

    //
    // Calculate and store primary cosmic-ray flux as a function of energy,
    // store the result in a binned PDF.
    //
    // Average the solar max and solar min spectra with weight that is sin(t/T+q0)
    // where T is the solar cycle period and q0 is an offset to put the max/min
    // in the right time. The incoming particle energy is then determined with
    // an accept-reject algorithm based on this PDF.
    //
    _maxPDF = 0.0;
    int Nbins = 10000;

    for(int i = 0; i < Nbins; i++) {
        double kine = pow(10.0, minl10 + (i + 0.5) * 1. / Nbins * (maxl10 - minl10));
        double kineMin = pow(10.0, minl10 + (i) * 1. / Nbins * (maxl10 - minl10));
        double kineMax = pow(10.0, minl10 + (i + 1.0) * 1. / Nbins * (maxl10 - minl10));
        double retValt = 0.;
        retValt += (1.0 - _cycle) * _solarMin->value(kine) + _cycle * _solarMax->value(kine);
        if(_wf != nullptr)
            retValt *= _wf->weight(kine);
        pdfValues.push_back(retValt * (kineMax - kineMin));
        if(retValt > _maxPDF)
            _maxPDF = retValt;
    }
    _maxPDF *= 1.1;

    std::vector<std::vector<double>> pdfVect;
    pdfVect.push_back(pdfValues);
    _cachedPdf = new CRYPdf("primaryTempPdf", _minEnergy, _maxEnergy, CRYPdf::LOG, "", pdfVect);
}
