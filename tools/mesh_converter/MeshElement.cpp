/**
 * @file
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "MeshElement.hpp"

#include <exception>

#include "core/utils/log.h"

#define MIN_VOLUME 1e-12

using namespace mesh_converter;

void MeshElement::calculate_volume() {
    if(dimension_ == 3) {
        Eigen::Matrix4d element_matrix;
        element_matrix << 1, 1, 1, 1, vertices_[0].x, vertices_[1].x, vertices_[2].x, vertices_[3].x, vertices_[0].y,
            vertices_[1].y, vertices_[2].y, vertices_[3].y, vertices_[0].z, vertices_[1].z, vertices_[2].z, vertices_[3].z;
        volume_ = (element_matrix.determinant()) / 6;
    }
    if(dimension_ == 2) {
        Eigen::Matrix3d element_matrix;
        element_matrix << 1, 1, 1, vertices_[0].y, vertices_[1].y, vertices_[2].y, vertices_[0].z, vertices_[1].z,
            vertices_[2].z;
        volume_ = (element_matrix.determinant()) / 2;
    }
}

double MeshElement::get_sub_volume(size_t index, Point& p) const {
    double volume = 0;
    if(dimension_ == 3) {
        Eigen::Matrix4d element_matrix;
        switch(index) {
        case 0:
            element_matrix << 1, 1, 1, 1, p.x, vertices_[1].x, vertices_[2].x, vertices_[3].x, p.y, vertices_[1].y,
                vertices_[2].y, vertices_[3].y, p.z, vertices_[1].z, vertices_[2].z, vertices_[3].z;
            break;
        case 1:
            element_matrix << 1, 1, 1, 1, vertices_[0].x, p.x, vertices_[2].x, vertices_[3].x, vertices_[0].y, p.y,
                vertices_[2].y, vertices_[3].y, vertices_[0].z, p.z, vertices_[2].z, vertices_[3].z;
            break;
        case 2:
            element_matrix << 1, 1, 1, 1, vertices_[0].x, vertices_[1].x, p.x, vertices_[3].x, vertices_[0].y,
                vertices_[1].y, p.y, vertices_[3].y, vertices_[0].z, vertices_[1].z, p.z, vertices_[3].z;
            break;
        case 3:
            element_matrix << 1, 1, 1, 1, vertices_[0].x, vertices_[1].x, vertices_[2].x, p.x, vertices_[0].y,
                vertices_[1].y, vertices_[2].y, p.y, vertices_[0].z, vertices_[1].z, vertices_[2].z, p.z;
            break;
        default:
            throw std::runtime_error("MeshElement::get_sub_volume: logic error, index must be 0 <= index <= 3");
            break;
        }
        volume = (element_matrix.determinant()) / 6;
    }
    if(dimension_ == 2) {
        Eigen::Matrix3d element_matrix;
        switch(index) {
        case 0:
            element_matrix << 1, 1, 1, p.y, vertices_[1].y, vertices_[2].y, p.z, vertices_[1].z, vertices_[2].z;
            break;
        case 1:
            element_matrix << 1, 1, 1, vertices_[0].y, p.y, vertices_[2].y, vertices_[0].z, p.z, vertices_[2].z;
            break;
        case 2:
            element_matrix << 1, 1, 1, vertices_[0].y, vertices_[1].y, p.y, vertices_[0].z, vertices_[1].z, p.z;
            break;
        default:
            throw std::runtime_error("MeshElement::get_sub_volume: logic error, index must be 0 <= index <= 2");
            break;
        }
        volume = (element_matrix.determinant()) / 2;
    }
    return volume;
}

double MeshElement::get_distance(size_t index, Point& qp) const {
    return unibn::L2Distance<Point>::compute(vertices_[index], qp);
}

bool MeshElement::isValid(double volume_cut, Point& qp) const {
    // Check if we should apply coplanar/colinear criterions:
    if(volume_cut > 0) {
        if(std::fabs(volume_) < MIN_VOLUME) {
            LOG(TRACE) << "Invalid tetrahedron, all vertices are " << (dimension_ == 3 ? "coplanar" : "colinear");
            return false;
        } else if(std::fabs(volume_) <= volume_cut) {
            LOG(TRACE) << "Invalid tetrahedron with volume " << std::fabs(volume_) << " smaller than volume cut "
                       << volume_cut;
            return false;
        }
    }

    for(size_t i = 0; i < dimension_ + 1; i++) {
        if(volume_ * get_sub_volume(i, qp) < 0) {
            LOG(TRACE) << "New mesh Point outside found element.";
            return false;
        }
    }
    return true;
}

Point MeshElement::getObservable(Point& qp) const {
    Point new_observable;
    for(size_t index = 0; index < dimension_ + 1; index++) {
        double sub_volume = get_sub_volume(index, qp);
        LOG(DEBUG) << "Sub volume " << index << ": " << sub_volume;
        new_observable.x = new_observable.x + (sub_volume * e_field_[index].x) / volume_;
        new_observable.y = new_observable.y + (sub_volume * e_field_[index].y) / volume_;
        new_observable.z = new_observable.z + (sub_volume * e_field_[index].z) / volume_;
    }
    LOG(DEBUG) << "Interpolated electric field: (" << new_observable.x << "," << new_observable.y << "," << new_observable.z
               << ")";
    return new_observable;
}

std::string MeshElement::print(Point& qp) const {
    std::stringstream stream;
    for(size_t index = 0; index < dimension_ + 1; index++) {
        stream << "Tetrahedron vertex (" << vertices_[index].x << ", " << vertices_[index].y << ", " << vertices_[index].z
               << ") - "
               << " Distance: " << get_distance(index, qp) << " - Electric field: (" << e_field_[index].x << ", "
               << e_field_[index].y << ", " << e_field_[index].z << ")" << std::endl;
    }
    stream << "Volume: " << volume_;
    return stream.str();
}
