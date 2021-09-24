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
#include "CRYGenerator.h"
#include "CRYParticle.h"
#include "CRYSetup.h"
#include "CRYUtils.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <assert.h>
#include <stdlib.h> // For Ubuntu Linux
#include <string.h> // For Ubuntu Linux

extern "C" {

CRYGenerator* gen;
double xoff0, yoff0, zoff0;

int cry_init_(double (*rng)(void)) {

    char* str;
    char crydatapath[256] = "";
    char crysetuppath[256] = "";

    if((str = getenv("CRYDATAPATH")) != NULL) {
        strncpy(crydatapath, str, strlen(str));
    } else {
        strncpy(crydatapath, "./data", strlen("./data"));
    }

    if((str = getenv("CRYSETUPPATH")) != NULL) {
        strncpy(crysetuppath, str, strlen(str));
    } else {
        strncpy(crysetuppath, ".", strlen("."));
    }

    strncat(crysetuppath, "/setup.file", strlen("/setup.file"));

    std::ifstream inputFile;
    inputFile.open(crysetuppath, std::ios::in);
    char buffer[1000];

    std::string setupString("");
    while(!inputFile.getline(buffer, 1000).eof()) {
        setupString.append(buffer);
        setupString.append(" ");
    }

    CRYSetup* setup = new CRYSetup(setupString, crydatapath);

    gen = new CRYGenerator(setup);

    setup->getUtils()->setRandomFunction(rng);

    xoff0 = setup->param(CRYSetup::xoffset);
    yoff0 = setup->param(CRYSetup::yoffset);
    zoff0 = setup->param(CRYSetup::zoffset);

    return 0;
}

int cry_smp_(double* erg,
             double* xxx,
             double* yyy,
             double* zzz,
             double* uuu,
             double* vvv,
             double* www,
             double* tme,
             int* pid,
             int* charge,
             int* npmax) {
    int npar = 0;

    std::vector<CRYParticle*>* ev = new std::vector<CRYParticle*>;
    ev->clear();
    gen->genEvent(ev);

    if(static_cast<int>(ev->size()) > *npmax) {
        std::cerr << "CRY::cry_fort: shower array size too small for shower of size " << ev->size() << std::endl;
        assert(0);
    }

    for(unsigned j = 0; j < ev->size(); j++) {
        erg[npar] = (*ev)[j]->ke();
        xxx[npar] = (*ev)[j]->x() + xoff0;
        yyy[npar] = (*ev)[j]->y() + yoff0;
        zzz[npar] = (*ev)[j]->z() + zoff0;
        uuu[npar] = (*ev)[j]->u();
        vvv[npar] = (*ev)[j]->v();
        www[npar] = (*ev)[j]->w();
        tme[npar] = (*ev)[j]->t();
        pid[npar] = (*ev)[j]->id();
        charge[npar] = (*ev)[j]->charge();

        npar++;
        delete(*ev)[j];
    }
    delete ev;

    return npar;
}

} // extern
