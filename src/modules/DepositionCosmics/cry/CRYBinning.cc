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

#include <assert.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h> // For Ubuntu Linux
#include <string.h> // For Ubuntu Linux
#include <string>
#include "CRYBinning.h"

CRYBinning::CRYBinning(std::string data) {

    _bins = new std::vector<double>;
    std::istringstream iss(data);
    std::string lhs;
    std::string rhs;

    std::getline(iss, lhs, '=');
    std::getline(iss, rhs, '=');

    // then lhs has the function name..
    std::istringstream iss2(lhs);
    std::string key = "";
    int c = 0;
    while(!iss2.eof()) {
        std::getline(iss2, key, ' ');
        if(0 != strcmp(key.c_str(), " ")) {
            c++;
            if(c != 1)
                _name.append(key);
        }
    }

    // then rhs has the data...
    std::string::size_type start = rhs.find("{");
    std::string::size_type stop = rhs.find("}");

    if(start == std::string::npos) {
        LOG(ERROR) << "CRY::CRYBinning: invalid binning - missing {. Data was:";
        LOG(ERROR) << data << std::endl;
        assert(0);
    }
    if(stop == std::string::npos) {
        LOG(ERROR) << "CRY::CRYBinning: invalid binning - missing }. Data was:";
        LOG(ERROR) << data << std::endl;
        assert(0);
    }

    std::string datums = rhs.substr(start + 1, stop - start - 1);
    std::istringstream iss3(datums);
    key = "";
    while(!iss3.eof()) {
        std::getline(iss3, key, ' ');
        if(key.length() > 0 && 0 != strcmp(key.c_str(), " ")) {
            _bins->push_back(atof(key.c_str()));
            if(_bins->size() > 1)
                if((*_bins)[_bins->size() - 1] <= (*_bins)[_bins->size() - 2]) {
                    LOG(ERROR) << "CRY::CRYBinning: Bins must be in monotonically increasing order. Data was:\n";
                    LOG(ERROR) << data << std::endl;
                    assert(0);
                }
        }
    }
    _size = _bins->size();
}

void CRYBinning::print(std::ostream& o, bool printData) {
    o << "Binning name: " << _name << std::endl;
    o << "  Bin   Edge location:\n";
    if(printData) {
        for(unsigned int i = 0; i < _size - 1; i++)
            o << "    " << i << "       " << (*_bins)[i] << std::endl;
        o << "            " << (*_bins)[_bins->size() - 1] << std::endl;
    }
}

int CRYBinning::bin(double value) {
    if(value < (*_bins)[0]) {
        LOG(ERROR) << "CRY::CRYBinning " << name() << ": Datum is in no bin. Stopping\n " << value << std::endl;
        assert(0);
    }

    long unsigned int i1 = 0;
    const long unsigned int divit = 5;
    long unsigned int i1Max = _size / divit;
    if(_size % divit == 0)
        i1Max--;
    for(i1 = 1; i1 <= i1Max;) {
        if((*_bins)[i1 * divit] > value)
            break;
        i1++;
    }

    long unsigned int t1 = std::min(1 + i1 * divit, _size);
    for(long unsigned int i = (i1 - 1) * divit; i < t1; i++)
        if((*_bins)[i] > value)
            return static_cast<int>(i - 1);

    return 0;
}
