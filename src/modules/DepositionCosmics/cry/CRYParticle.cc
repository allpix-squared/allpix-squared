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

#include "CRYParticle.h"

CRYParticle::CRYParticle(CRYId id, int charge, double KE) {
    _id = id;
    _charge = charge;
    _KE = KE;
}

CRYParticle::CRYParticle(const CRYParticle& part)
    : _id(part._id), _charge(part._charge), _KE(part._KE), _u(part._u), _v(part._v), _w(part._w), _x(part._x), _y(part._y),
      _z(part._z), _t(part._t) {}

void CRYParticle::fill(CRYParticle::CRYId& id,
                       int& q,
                       double& ke,
                       double& x,
                       double& y,
                       double& z,
                       double& u,
                       double& v,
                       double& w,
                       double& t) {

    id = _id;
    q = _charge;
    ke = _KE;
    x = _x;
    y = _y;
    z = _z;
    u = _u;
    v = _v;
    w = _w;
    t = _t;
}

//....return PDG number: http://pdg.lbl.gov/mc_particle_id_contents.html
//
#include <map>
int CRYParticle::PDGid() {
    static std::map<CRYId, int> PDGcode;
    PDGcode[Electron] = 11;
    PDGcode[Muon] = 13;
    PDGcode[Gamma] = 22;
    PDGcode[Neutron] = 2112;
    PDGcode[Proton] = 2212;
    PDGcode[Pion] = 211;
    PDGcode[Kaon] = 321;

    int anti = 1;
    if(_id == Electron || _id == Muon) {
        if(charge() > 0)
            anti = -1;
    } else {
        if(charge() < 0)
            anti = -1;
    }
    return anti * PDGcode[_id];
}
