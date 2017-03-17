// Global includes
#include <cstdlib>
#include <stdlib.h>

// Local includes
#include "DummyModule.hpp"

TestAlgorithm::TestAlgorithm(bool debugging) : Algorithm("TestAlgorithm") {
    m_debug = debugging;
}

void TestAlgorithm::initialise(Parameters* par) {

    // Pick up the run parameters
    parameters = par;
}

StatusCode TestAlgorithm::run(Clipboard* clipboard) {

    // Some output messages just to show that it runs
    debug << "Testing algorithm run stage" << endl;
    info << "Testing algorithm run stage" << endl;
    warning << "Testing algorithm run stage" << endl;

    // Get something from the storage element
    AllpixObjects* chargeDeposits = (AllpixObjects*)clipboard->get("chargeDeposits");
    if(chargeDeposits == NULL) {
        error << "There are no charge deposits on the clipboard" << endl;
        return Failure;
    }

    // Nothing went wrong in this event
    return Success;
}

void TestAlgorithm::finalise() {}
