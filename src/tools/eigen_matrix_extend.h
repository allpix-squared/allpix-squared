#include <G4ThreeVector.hh>
#include <Math/PositionVector3D.h>
#include <Math/SVector.h>
#include <TVector3.h>

operator G4ThreeVector() const { return G4ThreeVector(this->operator()(0), this->operator()(1), this->operator()(2)); }
operator TVector3() const { return TVector3(this->operator()(0), this->operator()(1), this->operator()(2)); }
operator ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>>() const {
    return ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>>(
        this->operator()(0), this->operator()(1), this->operator()(2));
}
template <typename T, unsigned int D> operator ROOT::Math::SVector<T, D>() const {
    ROOT::Math::SVector<T, D> svec;
    for(int i = 0; i < D; ++i) {
        svec(i) = this->operator()(i);
    }
    return svec;
}
