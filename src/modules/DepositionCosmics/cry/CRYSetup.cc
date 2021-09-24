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

#include "CRYSetup.h"
#include <assert.h>
#include <iostream>
#include <sstream>
#include <stdlib.h> // For Ubuntu Linux
#include "CRYData.h"

CRYSetup::CRYSetup(std::string configData, std::string dataDir) {

    _utils = new CRYUtils;

    // read the data!
    int nAlt = 3;
    int altitudes[3];
    altitudes[0] = 0;
    altitudes[1] = 2100;
    altitudes[2] = 11300;

    for(int i = 0; i < nAlt; i++) {
        std::ostringstream fileName;
        fileName << dataDir << "/cosmics_" << altitudes[i] << ".data";
        CRYData* data = new CRYData(fileName.str());
        _data[altitudes[i]] = data;
    }
    // data->print(std::cout);

    // param names
    _parmNames[CRYSetup::returnNeutrons] = "returnNeutrons";
    _parmNames[CRYSetup::returnProtons] = "returnProtons";
    _parmNames[CRYSetup::returnGammas] = "returnGammas";
    _parmNames[CRYSetup::returnElectrons] = "returnElectrons";
    _parmNames[CRYSetup::returnMuons] = "returnMuons";
    _parmNames[CRYSetup::returnPions] = "returnPions";
    _parmNames[CRYSetup::returnKaons] = "returnKaons";
    _parmNames[CRYSetup::subboxLength] = "subboxLength";
    _parmNames[CRYSetup::altitude] = "altitude";
    _parmNames[CRYSetup::latitude] = "latitude";
    _parmNames[CRYSetup::date] = "date";
    _parmNames[CRYSetup::nParticlesMin] = "nParticlesMin";
    _parmNames[CRYSetup::nParticlesMax] = "nParticlesMax";
    _parmNames[CRYSetup::xoffset] = "xoffset";
    _parmNames[CRYSetup::yoffset] = "yoffset";
    _parmNames[CRYSetup::zoffset] = "zoffset";

    // set some defaults
    _parms[CRYSetup::returnNeutrons] = 1.0;
    _parms[CRYSetup::returnProtons] = 1.0;
    _parms[CRYSetup::returnGammas] = 1.0;
    _parms[CRYSetup::returnElectrons] = 1.0;
    _parms[CRYSetup::returnMuons] = 1.0;
    _parms[CRYSetup::returnPions] = 1.0;
    _parms[CRYSetup::returnKaons] = 1.0;
    _parms[CRYSetup::subboxLength] = 100000000.0;
    _parms[CRYSetup::altitude] = 0.;
    _parms[CRYSetup::latitude] = 0.;
    std::string def_date = "1-1-2007";
    _parms[CRYSetup::date] = parseDate(def_date.c_str());
    _parms[CRYSetup::nParticlesMin] = 1;
    _parms[CRYSetup::nParticlesMax] = 1000000;
    _parms[CRYSetup::xoffset] = 0.;
    _parms[CRYSetup::yoffset] = 0.;
    _parms[CRYSetup::zoffset] = 0.;

    // ok - just split by whitespace into tokens...

    std::istringstream iss(configData);

    while(!iss.eof()) {
        std::string key = "";
        std::string value = "";

        //....find next keyword (single string delimited by a space)
        while(key == "" && !iss.eof())
            std::getline(iss, key, ' ');

        //....find next value (single string delimited by a space)
        while(value == "" && !iss.eof())
            std::getline(iss, value, ' ');

        if(value != "") {

            bool foundParm = false;

            for(CRYSetup::CRYParms i = CRYSetup::parmMin; i <= CRYSetup::parmMax; i = CRYSetup::CRYParms(i + 1)) {
                if(key == _parmNames[i]) {
                    _parms[i] = atof(value.c_str());
                    foundParm = true;
                    std::cout << "CRY::CRYSetup: Setting " << _parmNames[i] << " to ";
                    if(key == _parmNames[CRYSetup::date]) {
                        _parms[i] = parseDate(value.c_str());
                        std::cout << value << " (month-day-year)" << std::endl;
                    } else {
                        std::cout << _parms[i] << std::endl;
                    }
                }
            }
            if(!foundParm) {
                std::cerr << "CRY::CRYSetup: Found unknown parameter " << key << std::endl;
                std::cerr << "in configuration setup\n";
                assert(0);
            }
        }
    }
}

double CRYSetup::parseDate(std::string dt) {
    //  date is in the form month-day-year
    //
    //....Parse a date string and convert to a decimal number representing
    //    the year and a fraction of the year
    //  parseDate returns  year + (nbr_days / nbr_days_in_year)
    //      where: nbr_days = the number of days between the date and Jan 1
    //          e.g.  Date      nbr_days
    //               1 Jan             0
    //              31 Jan            30
    //               1 Feb            31
    //               1 Mar            59
    //                 ...           ...
    //              31 Dec           364 (For a non leap year)
    //
    std::istringstream issdate(dt);

    std::string text;
    std::getline(issdate, text, '-');
    int mm = atoi(text.c_str());

    std::getline(issdate, text, '-');
    int dd = atoi(text.c_str());

    std::getline(issdate, text, '-');
    double year = atof(text.c_str());
    int yy = atoi(text.c_str());

    bool gooddate = true;

    // initialize nbr of days in each month (first element is a dummy)
    //                     dummy, jan, feb, mar, apr, may, jun,
    int nbr_days_in_mo[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    //                            jul, aug, sep, oct, nov, dec

    double nbr_days_in_year = 365.0;

    if(isLeapYear(yy)) {
        nbr_days_in_mo[2] = 29;
        nbr_days_in_year = 366.0;
    } else {
        nbr_days_in_mo[2] = 28;
    }

    // Verify the date is valid
    if((mm < 1) || (mm > 12))
        gooddate = false;
    else if((dd < 1) || (dd > nbr_days_in_mo[mm]))
        gooddate = false;
    if(!gooddate) {
        std::cerr << "CRY::CRYSetup: The date " << dt << " is invalid.  The format is m-d-y" << std::endl;
        assert(0);
    }

    // The date is valid, continue

    // Create  an array of the cumulative number of days in previous months
    int cum_days[13];
    cum_days[0] = 0;
    for(int i = 1; i < 13; i++) {
        cum_days[i] = cum_days[i - 1] + nbr_days_in_mo[i - 1];
    }

    // Return  year + (nbr_days / days_in_year)
    int nbr_days = dd - 1 + cum_days[mm];
    return year + (static_cast<double>(nbr_days) / nbr_days_in_year);
}

//
//.... Determine if a year is a leap year
//
bool CRYSetup::isLeapYear(int yr)
// Returns true if yr is a leap year, false if it is not
{
    bool leap_year = false;
    if(yr % 400 == 0)
        leap_year = true;
    else if((yr % 4 == 0) && (yr % 100 != 0))
        leap_year = true;
    return leap_year;
}
