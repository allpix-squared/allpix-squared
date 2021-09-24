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

#include "CRYAbsFunction.h"
#include <assert.h>
#include <iostream>
#include <sstream>
#include <stdlib.h> // For Ubuntu Linux
#include <string.h> // For Ubuntu Linux
#include <string>
#include "CRYFunctionDict.h"
#include "CRYUtils.h"

CRYAbsFunction::CRYAbsFunction(std::string name, CRYFunctionDict::functype type, std::string rhs) {

    _params = new std::vector<double>;

    _type = type;
    _name = name;

    // then rhs has the data...
    std::string::size_type start = rhs.find("{");
    std::string::size_type stop = rhs.find("}");

    if(start == std::string::npos) {
        std::cerr << "CRY::CRYAbsFunction: invalid function - missing {. Data was:";
        std::cerr << rhs << std::endl;
        assert(0);
    }
    if(stop == std::string::npos) {
        std::cerr << "CRY::CRYAbsFunction: invalid function - missing }. Data was:";
        std::cerr << rhs << std::endl;
        assert(0);
    }

    std::string datums = rhs.substr(start + 1, stop - start - 1);
    std::istringstream iss3(datums);
    std::string key = "";
    while(!iss3.eof()) {
        std::getline(iss3, key, ' ');
        if(key.length() > 0 && 0 != strcmp(key.c_str(), " "))
            _params->push_back(atof(key.c_str()));
    }
}

void CRYAbsFunction::print(std::ostream& o, bool printData) {
    static CRYFunctionDict d;
    o << "Function name: " << _name << std::endl;
    o << "  Type " << d.type(_type) << std::endl;
    o << "  Parameter   Value:\n";
    for(unsigned int i = 0; i < _params->size(); i++)
        o << "      " << i << "        " << (*_params)[i] << std::endl;
}
