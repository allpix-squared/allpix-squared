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
//  CRYData.h
//
//  Created October, 2006
//
//  CRYData library of available functions, binnings, parameters, and pdfs
//

#ifndef CRYData_h
#define CRYData_h

#include <iostream>
#include <string>
#include <vector>

class CRYAbsFunction;
class CRYBinning;
class CRYPdf;
class CRYParameter;
class CRYParamI;

class CRYData {

public:
    // file containing definition of functions, binnings, etc
    // IMPROVEMENT: Check that file exists!
    CRYData(std::string file);

    // Call print on all datums. printData=true will print
    // the gory details of each of these data (eg, pdf values
    // for CRYPdf
    void print(std::ostream& o, bool printData = false);

    // Retrieve a function by name.
    // Returns 0 if not found
    CRYAbsFunction* getFunction(std::string name);
    std::vector<std::string> getFunctionList(std::string substr);

    // Retrieve a binning by name.
    // Returns 0 if not found
    CRYBinning* getBinning(std::string name);
    std::vector<std::string> getBinningList(std::string substr);

    // Retrieve a pdf by name.
    // Returns 0 if not found
    CRYPdf* getPdf(std::string name);
    std::vector<std::string> getPdfList(std::string substr);

    // Retrieve a parameter by name.
    // Returns 0 if not found
    CRYParameter* getParameter(std::string name);
    std::vector<std::string> getParameterList(std::string substr);

    // Retrieve a parameter by name.
    // Returns 0 if not found
    CRYParamI* getParamI(std::string name);
    std::vector<std::string> getParamIList(std::string substr);

private:
    // guts of file reading algorithm
    bool read();

    // list of defined functions
    std::vector<CRYAbsFunction*> _funcs;

    // list of defined binnings
    std::vector<CRYBinning*> _binnings;

    // list of defined pdfs
    std::vector<CRYPdf*> _pdfs;

    // list of defined parameters
    std::vector<CRYParameter*> _params;
    std::vector<CRYParamI*> _paramInts;

    // file to read from
    std::string _file;
};

#endif
