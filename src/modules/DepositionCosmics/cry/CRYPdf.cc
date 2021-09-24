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

#include "CRYPdf.h"
#include <assert.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h> // For Ubuntu Linux
#include <string.h> // For Ubuntu Linux
#include "CRYUtils.h"

CRYPdf::CRYPdf(std::string data) {

    _params = new std::vector<std::vector<double>>;
    _cdfs = new std::vector<std::vector<double>>;

    std::istringstream iss(data);
    std::string lhs;
    std::string rhs;

    std::getline(iss, lhs, '=');
    std::getline(iss, rhs, '=');

    // then lhs has the function name..
    _name = spaceTrimmer(lhs, 1);
    // but this name must have a :: that indicates the binning to use
    std::string pdfName;
    std::string pdfBinningName;

    std::string::size_type colLoc = _name.find(":");
    std::string::size_type obLoc = _name.find("[");
    std::string::size_type cbLoc = _name.find("]");

    if(colLoc == std::string::npos || _name[colLoc + 1] != ':') {
        std::cerr << "CRY::CRYPdf: Function must specify type. Data was:\n";
        std::cerr << data << std::endl;
        assert(0);
    }
    if(obLoc == std::string::npos) {
        std::cerr << "CRY::CRYPdf: Function must specify limits in []. Data was:\n";
        std::cerr << data << std::endl;
        assert(0);
    }
    if(cbLoc == std::string::npos) {
        std::cerr << "CRY::CRYPdf: Function must specify limits in []. Data was:\n";
        std::cerr << data << std::endl;
        assert(0);
    }

    std::istringstream issName(_name);
    std::getline(issName, pdfName, ':');
    // dummy get...
    std::getline(issName, pdfBinningName, ':');
    std::getline(issName, pdfBinningName, '[');

    _name = pdfName;
    _binningKey = pdfBinningName;

    std::string minStr, maxStr, typeStr;
    std::getline(issName, minStr, ',');
    std::getline(issName, maxStr, ',');
    std::getline(issName, typeStr, ']');
    typeStr = spaceTrimmer(typeStr);
    _min = atof(minStr.c_str());
    _max = atof(maxStr.c_str());
    _type = CRYPdf::UNKNOWN;
    if(typeStr == "dis")
        _type = CRYPdf::DISCRETE;
    if(typeStr == "lin")
        _type = CRYPdf::LINEAR;
    // for log binning - change min and max to log10 of the value entered for
    // easier calculations later on...
    if(typeStr == "log") {
        _type = CRYPdf::LOG;
        _min = log10(_min);
        _max = log10(_max);
    }
    if(_type == CRYPdf::UNKNOWN) {
        std::cerr << "CRY::CRYPdf: Unknown pdf type " << typeStr << " Data was:\n";
        std::cerr << data << std::endl;
        assert(0);
    }

    // ok - then there are two cases so far either one set of {}
    // or embedded {} within a {}

    std::string::size_type start = rhs.find("{");
    std::string::size_type stop = rhs.find("}");

    if(start == std::string::npos) {
        std::cerr << "CRY::CRYPdf: invalid function - missing {. Data was:";
        std::cerr << data << std::endl;
        assert(0);
    }
    if(stop == std::string::npos) {
        std::cerr << "CRY::CRYPdf: invalid function - missing }. Data was:";
        std::cerr << data << std::endl;
        assert(0);
    }

    std::string datums = rhs.substr(start + 1, stop - start - 1);
    std::string::size_type start2 = rhs.find("{", start + 1);

    // only one set of brackets
    if(start2 == std::string::npos) {
        readSetOfParams(datums);
    } else {
        // start=stop2;
        //   std::string::size_type stop2=data.find("{");
        while(start2 != std::string::npos) {
            datums = rhs.substr(start2 + 1, stop - start2 - 1);
            readSetOfParams(datums);
            start2 = rhs.find("{", start2 + 1);
            stop = rhs.find("}", start2);
        }
    }
}

CRYPdf::CRYPdf(std::string name,
               double minVal,
               double maxVal,
               pdfType pType,
               std::string binning,
               std::vector<std::vector<double>> values) {

    _min = minVal;
    _max = maxVal;
    if(pType == CRYPdf::LOG) {
        _min = log10(_min);
        _max = log10(_max);
    }

    _name = name;
    _type = pType;
    _binningKey = binning;
    _params = new std::vector<std::vector<double>>;
    _cdfs = new std::vector<std::vector<double>>;

    for(unsigned int i = 0; i < values.size(); i++) {
        _params->push_back(values[i]);
        std::vector<double> cdf;

        double sum = 0.;
        for(unsigned int j = 0; j < values[i].size(); j++)
            sum += values[i][j];

        double culm = 0.;
        for(unsigned int j = 0; j < values[i].size(); j++) {
            culm += (values[i][j] / sum);
            cdf.push_back(culm);
        }
        _cdfs->push_back(cdf);
    }
}

void CRYPdf::print(std::ostream& o, bool printData) {
    o << "PDF name: " << _name << std::endl;
    o << "  using binning key: " << _binningKey << std::endl;
    if(_type == CRYPdf::LOG)
        o << "  range: " << pow(10., _min) << " to " << pow(10., _max);
    else
        o << "  range: " << _min << " to " << _max;
    if(_type == CRYPdf::DISCRETE)
        o << " in discrete steps\n";
    if(_type == CRYPdf::LINEAR)
        o << " using linear bins\n";
    if(_type == CRYPdf::LOG)
        o << " using log10 bins\n";

    if(printData)
        for(unsigned int i = 0; i < _params->size(); i++) {
            o << i << "      ";
            for(unsigned int j = 0; j < (*_params)[i].size(); j++) {
                o << "  " << (*_params)[i][j];
            }
            o << std::endl;
        }
}

void CRYPdf::readSetOfParams(std::string data) {

    std::vector<double> results;

    std::istringstream iss(data);
    std::string key = "";
    while(!iss.eof()) {
        std::getline(iss, key, ' ');
        if(key.length() > 0 && 0 != strcmp(key.c_str(), " ")) {
            results.push_back(atof(key.c_str()));
        }
    }
    _params->push_back(results);

    double sum = 0.;
    for(unsigned int i = 0; i < results.size(); i++)
        sum += results[i];

    std::vector<double> cdf;
    double culm = 0.;
    for(unsigned int i = 0; i < results.size(); i++) {
        culm += (results[i] / sum);
        cdf.push_back(culm);
    }
    _cdfs->push_back(cdf);
}

double CRYPdf::draw(CRYUtils* utils, int bin) {
    double rand = utils->randomFlat();
    int cdfSize = (*_cdfs)[bin].size();

    int i1 = 0;
    int divit = int(sqrt((*_cdfs)[bin].size()));
    int i1Max = cdfSize / divit;
    if(cdfSize % divit == 0)
        i1Max--;
    for(i1 = 1; i1 <= i1Max;) {
        if((*_cdfs)[bin][i1 * divit] > rand)
            break;
        i1++;
    }

    for(int i = (i1 - 1) * divit; i < std::min(1.0 + i1 * divit, 1.0 * cdfSize); i++) {
        if((*_cdfs)[bin][i] > rand) {
            if(_type == CRYPdf::DISCRETE)
                return _min + i * (_max - _min) / (std::max(1.0, double(cdfSize) - 1));
            if(_type == CRYPdf::LINEAR)
                return _min + (i + utils->randomFlat()) * (_max - _min) / (cdfSize);
            if(_type == CRYPdf::LOG)
                return pow(10., _min + (i + utils->randomFlat()) * (_max - _min) / (cdfSize));
            std::cerr << "CRY::CRYPdf: Unknown pdf type? (impossible...)\n";
            assert(0);
        }
    }

    // should never get here
    std::cerr << "CRY::CRYPdf: Code has failed somehow (impossible...)\n";
    std::cerr << "CRY::CRYPdf: Name " << name() << " " << bin << " " << (*_cdfs)[bin][cdfSize - 1] << std::endl;
    assert(0);
    return 0.0;
}

std::string CRYPdf::spaceTrimmer(std::string str, int nskip) {
    std::istringstream iss2(str);
    std::string key = "";
    std::string retval = "";
    int c = 0;
    while(!iss2.eof()) {
        std::getline(iss2, key, ' ');
        if(0 != strcmp(key.c_str(), " ")) {
            c++;
            if(c > nskip)
                retval.append(key);
        }
    }
    return retval;
}

std::vector<double> CRYPdf::mean() {

    std::vector<double> retVal;
    for(unsigned int i = 0; i < _params->size(); i++) {
        double retValB = 0.;
        double integ = 0.;
        for(unsigned int j = 0; j < (*_params)[i].size(); j++) {

            double binCenter = 0.;
            if(_type == CRYPdf::DISCRETE)
                binCenter = _min + j * (_max - _min) / (std::max(1.0, double((*_cdfs)[i].size()) - 1));
            if(_type == CRYPdf::LINEAR)
                binCenter = _min + (j + 0.5) * (_max - _min) / ((*_cdfs)[i].size());
            if(_type == CRYPdf::LOG)
                binCenter = pow(10., _min + (j + 0.5) * (_max - _min) / ((*_cdfs)[i].size()));

            integ += (*_params)[i][j];
            retValB += binCenter * (*_params)[i][j];
        }
        if(integ > 0.)
            retValB /= integ;
        retVal.push_back(retValB);
    }
    return retVal;
}

std::vector<double> CRYPdf::sum() {

    std::vector<double> retVal;
    for(unsigned int i = 0; i < _params->size(); i++) {
        double integ = 0.;
        for(unsigned int j = 0; j < (*_params)[i].size(); j++) {
            integ += (*_params)[i][j];
        }
        retVal.push_back(integ);
    }
    return retVal;
}
