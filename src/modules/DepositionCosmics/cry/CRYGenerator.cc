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

#include "core/utils/log.h"

#include "CRYBinning.h"
#include "CRYData.h"
#include "CRYGenerator.h"
#include "CRYParamI.h"
#include "CRYParameter.h"
#include "CRYParticle.h"
#include "CRYPdf.h"
#include "CRYPrimary.h"
#include "CRYSetup.h"
#include "CRYUtils.h"
#include "CRYWeightFunc.h"

#include <assert.h>
#include <cmath>
#include <iostream>
#include <stdlib.h> // For Ubuntu Linux

CRYGenerator::CRYGenerator(CRYSetup* setup) {

    // random number generator

    _setup = setup;
    CRYData* data = _setup->getData(int(_setup->param(CRYSetup::altitude) + 0.1));

    if(data == nullptr) {
        LOG(ERROR) << "CRY::CRYGenerator: Data table not available for ";
        LOG(ERROR) << _setup->param(CRYSetup::altitude) << " meters.\n";
        LOG(ERROR) << "CRY::CRYGenerator: See data directory for available altitudes.\n";
        assert(0);
    }
    _utils = _setup->getUtils();

    // primary generator;
    _primary = new CRYPrimary(_utils, data, setup->param(CRYSetup::date), setup->param(CRYSetup::latitude));

    _primaryBinning = data->getBinning("primaryBins");
    _secondaryBinning = data->getBinning("secondaryBins");

    // Figure out the best box size from the ones available...
    std::vector<std::string> nPartPDFs = data->getPdfList("nParticles");
    if(nPartPDFs.size() == 0) {
        LOG(ERROR) << "CRY::CRYGenerator: Missing pdf for primary particles. Stopping " << std::endl;
        assert(0);
    }

    _subboxSize = _setup->param(CRYSetup::subboxLength);
    double bestBoxSize = 1.e99;
    unsigned int entry = 0;
    for(unsigned int i = 0; i < nPartPDFs.size(); i++) {
        double pdfBoxSize = atof(nPartPDFs[i].c_str());
        if(pdfBoxSize < bestBoxSize && pdfBoxSize >= _subboxSize) {
            bestBoxSize = pdfBoxSize;
            entry = i;
        }
    }

    // just take the biggest one if nothing is bigger than the subbox
    // specified
    if(bestBoxSize > 0.9e99) {
        bestBoxSize = 0;
        for(unsigned int i = 0; i < nPartPDFs.size(); i++) {
            double pdfBoxSize = atof(nPartPDFs[i].c_str());
            if(pdfBoxSize > bestBoxSize) {
                bestBoxSize = pdfBoxSize;
                entry = i;
            }
        }
    }

    _boxSize = bestBoxSize;

    std::string nPartPDF = "nParticles";
    nPartPDF.append(nPartPDFs[entry]);
    _nParticlesPDF = data->getPdf(nPartPDF);

    _particleFractionsPDF = data->getPdf("particleFractions");

    if(_primaryBinning == nullptr) {
        LOG(ERROR) << "CRY::CRYGenerator: Missing primary binning definition. Stopping " << std::endl;
        assert(0);
    }

    if(_secondaryBinning == nullptr) {
        LOG(ERROR) << "CRY::CRYGenerator: Missing secondary binning definition. Stopping " << std::endl;
        assert(0);
    }

    if(_particleFractionsPDF == nullptr) {
        LOG(ERROR) << "CRY::CRYGenerator: Missing pdf for particle fractions. Stopping " << std::endl;
        assert(0);
    }

    std::string latDistDef = "latDist";
    std::string timeDistDef = "timeDist";
    std::string cosThetaDistDef = "cosThetaDist";
    std::string chargeDef = "ChargeDist";

    for(CRYParticle::CRYId i = CRYParticle::CRYIdMin; i <= CRYParticle::CRYIdMax; i = CRYParticle::CRYId(i + 1)) {
        std::string name = _utils->partName(i);

        if(data->getParamI(name) != nullptr)
            _idDict[data->getParamI(name)->param()] = i;
        else {
            LOG(ERROR) << "CRY::CRYGenerator: Missing paramI for particle " << name << ". Stopping " << std::endl;
            assert(0);
        }

        std::string keKey = name;
        keKey.append("KEDist");
        _kePdfs[i] = data->getPdf(keKey);

        std::string latKey = name;
        latKey.append("LatDist");
        std::string timeKey = name;
        timeKey.append("TimeDist");
        std::string cosThetaKey = name;
        cosThetaKey.append("CosThetaDist");
        std::string chargeKey = name;
        chargeKey.append("ChargeDist");

        CRYPdf* tPdf = data->getPdf(latKey);
        if(tPdf == nullptr) {
            _latPdfs[i] = data->getPdf(latDistDef);
        } else
            _latPdfs[i] = tPdf;

        // horrible hack - set the latpdf min and max to the box size by hand
        if(_latPdfs[i] != nullptr) {
            _latPdfs[i]->setMin(-1.0 * _boxSize / 2.0);
            _latPdfs[i]->setMax(_boxSize / 2.0);
        }

        tPdf = data->getPdf(timeKey);
        if(tPdf == nullptr) {
            _timePdfs[i] = data->getPdf(timeDistDef);
        } else
            _timePdfs[i] = tPdf;

        tPdf = data->getPdf(cosThetaKey);
        if(tPdf == nullptr) {
            _cosThetaPdfs[i] = data->getPdf(cosThetaDistDef);
        } else
            _cosThetaPdfs[i] = tPdf;

        tPdf = data->getPdf(chargeKey);
        if(tPdf == nullptr) {
            _chargePdfs[i] = data->getPdf(chargeDef);
        } else
            _chargePdfs[i] = tPdf;

        if(_timePdfs[i] == nullptr) {
            LOG(ERROR) << "CRY::CRYGenerator: Missing time pdf for " << name << std::endl;
            assert(0);
        }
        if(_latPdfs[i] == nullptr) {
            LOG(ERROR) << "CRY::CRYGenerator: Missing lat pdf for " << name << std::endl;
            assert(0);
        }
        if(_kePdfs[i] == nullptr) {
            LOG(ERROR) << "CRY::CRYGenerator: Missing kinetic energy pdf for " << name << std::endl;
            assert(0);
        }

        if(_cosThetaPdfs[i] == nullptr) {
            LOG(ERROR) << "CRY::CRYGenerator: Missing cos theta pdf for " << name << std::endl;
            assert(0);
        }
    }

    _tallyList[CRYParticle::Neutron] = _setup->param(CRYSetup::returnNeutrons) > 0.5 ? true : false;
    _tallyList[CRYParticle::Proton] = _setup->param(CRYSetup::returnProtons) > 0.5 ? true : false;
    _tallyList[CRYParticle::Gamma] = _setup->param(CRYSetup::returnGammas) > 0.5 ? true : false;
    _tallyList[CRYParticle::Pion] = _setup->param(CRYSetup::returnPions) > 0.5 ? true : false;
    _tallyList[CRYParticle::Electron] = _setup->param(CRYSetup::returnElectrons) > 0.5 ? true : false;
    _tallyList[CRYParticle::Muon] = _setup->param(CRYSetup::returnMuons) > 0.5 ? true : false;
    _tallyList[CRYParticle::Kaon] = _setup->param(CRYSetup::returnKaons) > 0.5 ? true : false;

    _maxParticles = int(_setup->param(CRYSetup::nParticlesMax));
    _minParticles = int(_setup->param(CRYSetup::nParticlesMin));

    if(_maxParticles < 1 || _maxParticles < _minParticles) {
        LOG(ERROR) << "CRY::CRYGenerator: Nonsense settings for min/max particles: ";
        LOG(ERROR) << _minParticles << " " << _maxParticles << std::endl;
        assert(0);
    }

    // figure out dt between showers
    std::vector<double> primaryPartialRates = _primary->partialRates(_primaryBinning->bins());
    std::vector<double> averageMultInBox = _nParticlesPDF->mean();
    // this is # of particles per primary!
    std::vector<double> secondariesPerShower = _particleFractionsPDF->sum();
    double boxArea = _boxSize * _boxSize;

    // Need to create a PDF out of others to determine the
    // fraction of time in each primary bin that there are >0 particles
    //
    std::vector<double> fractionWithParticles;

    for(unsigned int i = 0; i < primaryPartialRates.size(); i++) {
        if(averageMultInBox[i] > 0.) {
            // primaries per m2 per sec * secondaries per primary / (secondaries per box/ m2 per box)
            fractionWithParticles.push_back(secondariesPerShower[i] / averageMultInBox[i]);
        } else {
            fractionWithParticles.push_back(0.);
        }
    }

    _primaryWeighting = new CRYWeightFunc(_primaryBinning, fractionWithParticles);
    _primary->setWeightFunc(boxArea, _primaryWeighting);

    _primaryPart = nullptr;
}

std::vector<CRYParticle*>* CRYGenerator::genEvent() {
    std::vector<CRYParticle*>* retList = nullptr;
    genEvent(retList);
    return retList;
}

void CRYGenerator::genEvent(std::vector<CRYParticle*>* retList) {
    if(retList == nullptr)
        retList = new std::vector<CRYParticle*>;

    int pBin = 0, sBin = 0;

    do {
        delete _primaryPart;
        _primaryPart = _primary->getPrimary();
        pBin = _primaryBinning->bin(_primaryPart->ke());
        int nSecondary = int(_nParticlesPDF->draw(_utils, pBin));

        if(nSecondary == 0) {
            continue;
        }
        for(int i = 0; i < nSecondary; i++) {
            double temp = _particleFractionsPDF->draw(_utils, pBin);
            int pType = int(temp);
            CRYParticle::CRYId idSec = _idDict[pType];

            // stop -- do we want to tally this type of particle???
            if(!(_tallyList[idSec]))
                continue;

            double keSecondary = _kePdfs[idSec]->draw(_utils, pBin);

            // Now sample lateral distribution, for now is just flat inside the box
            //   boxSize is size of box in data files that is just big enough to enclose subboxSize
            //   subboxSize is the user selected output box size
            //
            // could use this, which uses the latPdf to generate flat distribution
            //
            double xPosSecondary = _latPdfs[idSec]->draw(_utils, pBin);
            double yPosSecondary = _latPdfs[idSec]->draw(_utils, pBin);
            ;
            //
            // instead just sample directly
            //
            // double xPosSecondary=_utils->randomFlat(-0.5*_boxSize,0.5*_boxSize);
            // double yPosSecondary=_utils->randomFlat(-0.5*_boxSize,0.5*_boxSize);

            //....only keep this secondary if it is inside the user-selected box
            if(fabs(xPosSecondary) > 0.5 * _subboxSize)
                continue;
            if(fabs(yPosSecondary) > 0.5 * _subboxSize)
                continue;

            //....sample the time distribution
            sBin = _secondaryBinning->bin(keSecondary);
            double timeSecondary = _primary->timeSimulated() + _timePdfs[idSec]->draw(_utils, sBin);

            int charge = static_cast<int>(_chargePdfs[idSec]->draw(_utils, sBin));

            double u, v, w;
            w = _cosThetaPdfs[idSec]->draw(_utils, sBin);

            double maxV = std::sqrt(1.0 - w * w);
            double tphi = _utils->randomFlat() * 2.0 * M_PI;
            v = maxV * sin(tphi);
            u = maxV * cos(tphi);

            // make secondary and add it to the list
            CRYParticle* daug = new CRYParticle(idSec, charge, keSecondary);
            daug->setPosition(xPosSecondary, yPosSecondary, 0.);
            daug->setTime(timeSecondary);
            daug->setDirection(u, v, w);

            // if the user has set a limit in the number of particles
            if(static_cast<int>(retList->size()) < _maxParticles)
                retList->push_back(daug);
            else
                delete daug;
        }

    } while(static_cast<int>(retList->size()) < _minParticles);
}
