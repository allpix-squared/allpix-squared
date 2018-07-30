#include "MeshElement.hpp"

#include "core/utils/log.h"

#define MIN_VOLUME 1e-12

using namespace mesh_converter;

void MeshElement::setVertices(std::vector<Point>& new_vertices) {
    if(vertices_.size() != new_vertices.size()) {
        LOG(ERROR) << "Invalid vertices vector";
        return;
    }
    for(size_t index = 0; index < new_vertices.size(); index++) {
        vertices_[index] = new_vertices[index];
    }
}

void MeshElement::setVertex(size_t index, Point& new_vertice) {
    vertices_[index] = new_vertice;
}

Point MeshElement::getVertex(size_t index) {
    return vertices_[index];
}

void MeshElement::setVerticesField(std::vector<Point>& new_observable) {
    if(vertices_.size() != new_observable.size()) {
        LOG(ERROR) << "Invalid field vector";
        return;
    }
    for(size_t index = 0; index < 4; index++) {
        e_field_[index] = new_observable[index];
    }
}

void MeshElement::setVertexField(size_t index, Point& new_observable) {
    e_field_[index] = new_observable;
}

Point MeshElement::getVertexProperty(size_t index) {
    return e_field_[index];
}

void MeshElement::setDimension(int dimension) {
    dimension_ = dimension;
}

int MeshElement::getDimension() {
    return dimension_;
}

double MeshElement::getVolume() {
    double volume = 0;
    if(this->getDimension() == 3) {
        Eigen::Matrix4d element_matrix;
        element_matrix << 1, 1, 1, 1, vertices_[0].x, vertices_[1].x, vertices_[2].x, vertices_[3].x, vertices_[0].y,
            vertices_[1].y, vertices_[2].y, vertices_[3].y, vertices_[0].z, vertices_[1].z, vertices_[2].z, vertices_[3].z;
        volume = (element_matrix.determinant()) / 6;
    }
    if(this->getDimension() == 2) {
        Eigen::Matrix3d element_matrix;
        element_matrix << 1, 1, 1, vertices_[0].y, vertices_[1].y, vertices_[2].y, vertices_[0].z, vertices_[1].z,
            vertices_[2].z;
        volume = (element_matrix.determinant()) / 2;
    }
    return volume;
}

double MeshElement::getDistance(size_t index, Point& qp) {
    return unibn::L2Distance<Point>::compute(vertices_[index], qp);
}

bool MeshElement::validElement(double volume_cut, Point& qp) {
    if(this->getVolume() < MIN_VOLUME) {
        LOG(TRACE) << "Invalid tetrahedron with coplanar(3D)/colinear(2D) vertices.";
        return false;
    }
    if(std::fabs(this->getVolume()) <= volume_cut) {
        LOG(TRACE) << "Tetrahedron volume smaller than volume cut.";
        return false;
    }

    Eigen::Matrix4d sub_tetra_matrix;
    for(size_t i = 0; i < static_cast<size_t>(this->getDimension()) + 1; i++) {
        std::vector<Point> sub_vertices = vertices_;
        sub_vertices[i] = qp;
        MeshElement sub_tetrahedron(sub_vertices);
        sub_tetrahedron.setDimension(this->getDimension());
        double tetra_volume = sub_tetrahedron.getVolume();
        if(this->getVolume() * tetra_volume >= 0) {
            continue;
        }
        if(this->getVolume() * tetra_volume < 0) {
            LOG(TRACE) << "New mesh Point outside found element.";
            return false;
        }
    }
    return true;
}

Point MeshElement::getObservable(Point& qp) {
    Point new_observable;
    Eigen::Matrix4d sub_tetra_matrix;
    for(size_t index = 0; index < static_cast<size_t>(this->getDimension()) + 1; index++) {
        auto sub_vertices = vertices_;
        sub_vertices[index] = qp;
        MeshElement sub_tetrahedron(sub_vertices);
        sub_tetrahedron.setDimension(this->getDimension());
        double sub_volume = sub_tetrahedron.getVolume();
        LOG(DEBUG) << "Sub volume " << index << ": " << sub_volume;
        new_observable.x = new_observable.x + (sub_volume * e_field_[index].x) / this->getVolume();
        new_observable.y = new_observable.y + (sub_volume * e_field_[index].y) / this->getVolume();
        new_observable.z = new_observable.z + (sub_volume * e_field_[index].z) / this->getVolume();
    }
    LOG(DEBUG) << "Interpolated electric field: (" << new_observable.x << "," << new_observable.y << "," << new_observable.z
               << ")" << std::endl;
    return new_observable;
}

void MeshElement::printElement(Point& qp) {
    for(size_t index = 0; index < static_cast<size_t>(this->getDimension()) + 1; index++) {
        LOG(DEBUG) << "Tetrahedron vertex " << indices_[index] << " (" << vertices_[index].x << ", " << vertices_[index].y
                   << ", " << vertices_[index].z << ") - "
                   << " Distance: " << this->getDistance(index, qp) << " - Electric field: (" << e_field_[index].x << ", "
                   << e_field_[index].y << ", " << e_field_[index].z << ")";
    }
    LOG(DEBUG) << "Volume: " << this->getVolume();
}
