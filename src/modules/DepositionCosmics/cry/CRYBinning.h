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
//  CRYBinning.h
//
//  Created October, 2006
//
//  CRYBinning holds the definition of a binning structure
//  to be used by one or more CRYPdf objects
//

#ifndef CRYBinning_h
#define CRYBinning_h

#include <iostream>
#include <string>
#include <vector>

class CRYBinning {

public:
    // Nominal constructor
    // Data format is expected to be
    //
    // binning myBinning = { 1.0 2.0 3.0 8.0}
    //
    // with monotonically increaing values describing
    // the bin edges. The last value is the upper limit
    // of the last bin, so there is one more entry required
    // than the # of bins.
    CRYBinning(std::string data = "");

    // Destructor. CRYBinning owns _bins so delete it
    ~CRYBinning() {
        delete _bins;
        _bins = nullptr;
    }

    // Print to cout the name and optionally (according to printData)
    // the binning as defined by _bins
    void print(std::ostream& o, bool printData = false);

    // Returns the key
    std::string name() { return _name; }

    // Direct access to the binning defintion
    const std::vector<double>* bins() const { return _bins; }

    // Given x (value), determine the corresponding bin
    // Return value is 0..N
    // x is outside of the defined range, an assert will be thrown
    int bin(double value);

    // Get the boundaries of this binning
    double min() { return (*_bins)[0]; }
    double max() { return (*_bins)[_bins->size() - 1]; }

private:
    // Key for this binning structur
    std::string _name;
    // The bin defintions - For N bins, there are N+1 values
    // corresponding to the lower and upper limits of each bin
    // and assuming no gaps between bins
    std::vector<double>* _bins;

    // Store the # of bins for conviencience only (_bins->size())
    unsigned int _size;
};

#endif
