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
//  CRYAbsFunction.h
//
//  Created October, 2006
//
//  CRYAbsFunction abstract function class

#ifndef CRYAbsFunction_h
#define CRYAbsFunction_h

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "CRYFunctionDict.h"

class CRYAbsFunction {
public:
    // Short list of currently known function types

    // Nominal constructor. Expected data format
    // function myFunc::PrimarySpectum1 = { 1. 2. 3. 4.}
    // Where PrimarySpectrum1 corresponds to the function
    // type defined by the enums (and corresponding enum<->key
    // map defined in the CRYFunction constructor
    CRYAbsFunction(std::string name, CRYFunctionDict::functype type, std::string data = "");

    // We own _params. Delete it
    virtual ~CRYAbsFunction() { delete _params; }

    // Print the function name, type and optionally
    //(depending on printData the function parameters
    void print(std::ostream& o, bool printData = false);

    // The function key
    std::string name() { return _name; }

    // The function type
    CRYFunctionDict::functype type() { return _type; }

    // Direct access to the parameter vector
    const std::vector<double>* params() { return _params; }

    // Evaluate the function given an input or vector of inputs
    // These are defined in the derived classes
    virtual double value(double x) = 0;
    virtual double value(std::vector<double> x) = 0;

protected:
    // Parameter vector
    std::vector<double>* _params;

private:
    // Function name (key)
    std::string _name;

    // The function type - See enums above for known function types
    CRYFunctionDict::functype _type;
};

#endif
