#ifndef TESTALGORITHM_H
#define TESTALGORITHM_H 1

// Global includes
#include <iostream>

// ROOT includes
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"

// Local includes
#include "Algorithm.hpp"

class TestAlgorithm : public Algorithm {

public:
    // Constructors and destructors
    TestAlgorithm(bool);
    ~TestAlgorithm() {}

    // Functions
    void       initialise(Parameters*);
    StatusCode run(Clipboard*);
    void       finalise();
};

#endif // TESTALGORITHM_H
