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
//  CRYdf.h
//
//  Created October, 2006
//
//  CRYPdf holds the definition a binned probability density function
//

#ifndef CRYPdf_h
#define CRYPdf_h

#include <iostream>
#include <string>
#include <vector>

class CRYUtils;

class CRYPdf {
public:
    // Types of PDFs expected:
    //  For PDFs with 2 binning dimensions, this is relevant
    //  for the inner dimension. The outer is specified via
    //  CRYBinning structure as described below
    //     DISCRETE : Only discrete values are returned
    //       Possible values are in equal steps between
    //       min and max values specified
    //     LINEAR : Equal steps between specified min and max
    //       Sample flat within bin
    //     LOG : Log10 steps between specified min and max
    //       Sample flat within bin
    enum pdfType { DISCRETE, LINEAR, LOG, UNKNOWN };

    // Nominal constructor
    // expected data format
    //  pdf myPDF::myBinning[1,5,dis] = {
    //    { 0.1 0.3 0.5 0.6 0.7 }
    //    { 0.1 0.3 0.5 0.6 0.7 }
    //  }
    // where the number of {....} sets must match
    // the # of bins defined in the specified binning
    //
    // The values in [] define the minimum and maximum
    // in the inner dimension of the pdf, as well as the
    // way these values should be interpreted (see description
    // above regarding pdfType
    //
    CRYPdf(std::string data = "");

    CRYPdf(std::string name,
           double minVal,
           double maxVal,
           pdfType pType,
           std::string binning,
           std::vector<std::vector<double>> values);

    // Nominal destructor
    ~CRYPdf() {
        delete _params;
        delete _cdfs;
    }

    // Print pdf information
    // Optionally (based on printData value) print PDF data
    void print(std::ostream& o, bool printData = false);

    // Function key
    std::string name() { return _name; }

    // Return binning key for this pdf (key for CRYBinning object)
    std::string key() { return _binningKey; }

    // Direct access to PDF values
    const std::vector<std::vector<double>>* params() { return _params; }

    // Given primary binning, draw random value from PDF
    double draw(CRYUtils* utils, int bin);

    // Compute mean of PDF. Value is returned for each bin in primary
    // binning if PDF definition has two dimensions
    std::vector<double> mean();

    // Compute sum of PDF. Value is returned for each bin in primary
    // binning if PDF definition has two dimensions
    std::vector<double> sum();

    // hack - add accessors to set min and max..
    void setMin(double min) { _min = min; }
    void setMax(double max) { _max = max; }

private:
    void readSetOfParams(std::string data);
    std::string spaceTrimmer(std::string str, int nskip = 0);

    std::string _name;
    std::string _binningKey;
    double _min;
    double _max;
    CRYPdf::pdfType _type;

    std::vector<std::vector<double>>* _params;
    std::vector<std::vector<double>>* _cdfs; // normalized to 1
};

#endif
