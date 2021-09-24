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
//  CRYParticle.h
//
//  Created October, 2006
//
//  CRYParticle: Definition of particle to be returned by CRY library
//

#ifndef CRYParticle_h
#define CRYParticle_h

#include <string>

class CRYParticle {

public:
    // Known particle types
    // Corresponding names (strings) defined in CRYUtils class
    enum CRYId { CRYIdMin, Neutron = CRYIdMin, Proton, Pion, Kaon, Muon, Electron, Gamma, CRYIdMax = Gamma };

    // Basic constructor
    // ID is particle type (see CRYId enum list)
    // Charge (+1,0,-1)
    // KE is kinetic energy
    //   Units will match units of data tables
    //   Currently MeV
    CRYParticle(CRYId id, int charge, double KE);
    CRYParticle(const CRYParticle& part);

    // Set location of particle
    //   Units will match units of data tables
    //   Currently m
    void setPosition(double x, double y, double z) {
        _x = x;
        _y = y;
        _z = z;
    }

    // Set direction of particle
    void setDirection(double u, double v, double w) {
        _u = u;
        _v = v;
        _w = w;
    }

    // Set time of particle
    //   Units will match units of data tables
    //   Currently seconds
    void setTime(double t) { _t = t; }

    // accessors

    // Return all of the defined parameters
    // kinetic energy
    //   Units will match units of data tables
    //   Currently MeV
    double ke() { return _KE; }

    //   Location
    //   Units will match units of data tables
    //   Currently m
    double x() { return _x; }
    double y() { return _y; }
    double z() { return _z; }

    // Direction cosines
    double u() { return _u; }
    double v() { return _v; }
    double w() { return _w; }
    int charge() { return _charge; }

    // Time
    //   Units will match units of data tables
    //   Currently seconds
    double t() { return _t; }

    // Particle type
    CRYParticle::CRYId id() { return _id; }

    // Return PDG number (http://pdg.lbl.gov/mc_particle_id_contents.html)
    int PDGid();

    void fill(CRYParticle::CRYId& id,
              int& q,
              double& ke,
              double& x,
              double& y,
              double& z,
              double& u,
              double& v,
              double& w,
              double& t);

private:
    CRYId _id;
    int _charge;
    double _KE;
    double _u, _v, _w;
    double _x, _y, _z;
    double _t;
};

#endif
