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

#include "CRYData.h"
#include "CRYAbsFunction.h"
#include "CRYBinning.h"
#include "CRYFunctionDict.h"
#include "CRYParamI.h"
#include "CRYParameter.h"
#include "CRYPdf.h"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h> // For Ubuntu Linux

CRYData::CRYData(std::string file) {
    _file = file;
    bool readOk = read();
    if(!readOk) {
        std::cerr << "CRY::CRYData: Error reading " << _file << "....  Stopping\n";
        assert(0);
    }
}

bool CRYData::read() {
    CRYFunctionDict fDict;

    //  std::cout << "CRY::CRYData: Reading data file " << _file << std::endl;

    std::ifstream file;
    file.open(_file.c_str(), std::ios::in);

    if(!file.is_open())
        return false;

    char buffer[1000];

    std::string fileContents("");
    while(!file.getline(buffer, 1000).eof()) {
        int i = 0;
        for(i = 0; i < file.gcount() - 1; i++)
            if(buffer[i] == '%') {
                break;
            }
        if(i > 0) {
            std::string thisLine(buffer, i);
            fileContents.append(thisLine);
        }
    }

    std::istringstream iss(fileContents);
    std::string token;
    while(!iss.eof()) {
        std::getline(iss, token, ';');

        // Ok- then get the first part of the token
        // and send the rest to the right class...
        std::istringstream iss2(token);
        std::string key = "";
        int lastGetPosition = std::ios::beg;
        while(key == "" && !iss2.eof()) {
            std::getline(iss2, key, ' ');
            if(key == "")
                lastGetPosition = iss2.tellg();
        }
        iss2.seekg(lastGetPosition);
        std::string token2;
        std::getline(iss2, token2, ';');

        if(0 == strncmp(key.c_str(), "function", 8)) {
            _funcs.push_back(fDict.function(token2));
        }

        if(0 == strncmp(key.c_str(), "pdf", 3)) {
            _pdfs.push_back(new CRYPdf(token2));
        }

        if(0 == strncmp(key.c_str(), "binning", 7)) {
            _binnings.push_back(new CRYBinning(token2));
        }

        if(0 == strncmp(key.c_str(), "parameter", 9)) {
            _params.push_back(new CRYParameter(token2));
        }

        if(0 == strncmp(key.c_str(), "paramInt", 8)) {
            _paramInts.push_back(new CRYParamI(token2));
        }
    }

    return true;
}

void CRYData::print(std::ostream& o, bool printData) {
    o << "Begin CRYData print ==================================\n";
    o << "Number of functions defined: " << _funcs.size() << std::endl;
    for(unsigned int i = 0; i < _funcs.size(); i++)
        _funcs[i]->print(o, printData);

    o << "\nNumber of binnings defined: " << _binnings.size() << std::endl;
    for(unsigned int i = 0; i < _binnings.size(); i++)
        _binnings[i]->print(o, printData);

    o << "\nNumber of pdfs defined: " << _pdfs.size() << std::endl;
    for(unsigned int i = 0; i < _pdfs.size(); i++)
        _pdfs[i]->print(o, printData);

    o << "\nNumber of parameters defined: " << _params.size() << std::endl;
    for(unsigned int i = 0; i < _params.size(); i++)
        _params[i]->print(o, printData);
    for(unsigned int i = 0; i < _paramInts.size(); i++)
        _paramInts[i]->print(o, printData);

    o << "End   CRYData print ==================================\n";
}

CRYAbsFunction* CRYData::getFunction(std::string name) {
    for(unsigned int i = 0; i < _funcs.size(); i++)
        if(_funcs[i]->name() == name)
            return _funcs[i];
    return 0;
}
CRYBinning* CRYData::getBinning(std::string name) {
    for(unsigned int i = 0; i < _binnings.size(); i++)
        if(_binnings[i]->name() == name)
            return _binnings[i];
    return 0;
}
CRYPdf* CRYData::getPdf(std::string name) {
    for(unsigned int i = 0; i < _pdfs.size(); i++)
        if(_pdfs[i]->name() == name)
            return _pdfs[i];
    return 0;
}

CRYParameter* CRYData::getParameter(std::string name) {
    for(unsigned int i = 0; i < _params.size(); i++)
        if(_params[i]->name() == name)
            return _params[i];
    return 0;
}

CRYParamI* CRYData::getParamI(std::string name) {
    for(unsigned int i = 0; i < _paramInts.size(); i++)
        if(_paramInts[i]->name() == name)
            return _paramInts[i];
    return 0;
}

std::vector<std::string> CRYData::getParameterList(std::string substr) {
    std::vector<std::string> retVal;
    for(unsigned int i = 0; i < _params.size(); i++)
        if(_params[i]->name().substr(0, substr.length()) == substr)
            retVal.push_back(_params[i]->name().substr(substr.length(), _params[i]->name().length() - substr.length()));

    return retVal;
}

std::vector<std::string> CRYData::getParamIList(std::string substr) {
    std::vector<std::string> retVal;
    for(unsigned int i = 0; i < _paramInts.size(); i++)
        if(_paramInts[i]->name().substr(0, substr.length()) == substr)
            retVal.push_back(
                _paramInts[i]->name().substr(substr.length(), _paramInts[i]->name().length() - substr.length()));

    return retVal;
}

std::vector<std::string> CRYData::getBinningList(std::string substr) {
    std::vector<std::string> retVal;
    for(unsigned int i = 0; i < _binnings.size(); i++)
        if(_binnings[i]->name().substr(0, substr.length()) == substr)
            retVal.push_back(_binnings[i]->name().substr(substr.length(), _binnings[i]->name().length() - substr.length()));

    return retVal;
}

std::vector<std::string> CRYData::getPdfList(std::string substr) {
    std::vector<std::string> retVal;
    for(unsigned int i = 0; i < _pdfs.size(); i++)
        if(_pdfs[i]->name().substr(0, substr.length()) == substr)
            retVal.push_back(_pdfs[i]->name().substr(substr.length(), _pdfs[i]->name().length() - substr.length()));

    return retVal;
}

std::vector<std::string> CRYData::getFunctionList(std::string substr) {
    std::vector<std::string> retVal;
    for(unsigned int i = 0; i < _funcs.size(); i++)
        if(_funcs[i]->name().substr(0, substr.length()) == substr)
            retVal.push_back(_funcs[i]->name().substr(substr.length(), _funcs[i]->name().length() - substr.length()));

    return retVal;
}
