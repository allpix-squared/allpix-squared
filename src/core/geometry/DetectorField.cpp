#include "DetectorField.hpp"

namespace allpix {

    /*
     * Vector field template specialization of helper function for field flipping
     */
    template <> void flip_vector_components<ROOT::Math::XYZVector>(ROOT::Math::XYZVector& vec, bool x, bool y) {
        if(x) {
            vec.SetX(-vec.x());
        }
        if(y) {
            vec.SetY(-vec.y());
        }
    }

    /*
     * Scalar field template specialization of helper function for field flipping
     * Here, no inversion of the field components is required
     */
    template <> void flip_vector_components<double>(double&, bool, bool) {}
} // namespace allpix
