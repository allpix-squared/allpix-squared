#include "DetectorField.hpp"

using namespace allpix;

template <> void allpix::flip_vector_components<ROOT::Math::XYZVector>(ROOT::Math::XYZVector& vec, bool x, bool y) {
    if(x) {
        vec.SetX(-vec.x());
    }
    if(y) {
        vec.SetY(-vec.y());
    }
}

template <> void allpix::flip_vector_components<double>(double&, bool, bool) {}
