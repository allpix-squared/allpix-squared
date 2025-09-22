/**
 * @file
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MESHELEMENT_H
#define ALLPIX_MESHELEMENT_H

#include <Eigen/Eigen>
#include <array>
#include <cmath>
#include <utility>

#include "core/utils/log.h"
#include "octree/Octree.hpp"

namespace mesh_converter {
    // Simple point class to store data
    class Point {
    public:
        Point() noexcept = default;
        Point(double px, double py, double pz) noexcept : x(px), y(py), z(pz), dim(3) {};
        Point(double py, double pz) noexcept : y(py), z(pz), dim(2) {};

        bool isFinite() const { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z); }

        double x{0}, y{0}, z{0};
        unsigned int dim{0};

        friend std::ostream& operator<<(std::ostream& out, const Point& pt) {
            out << "(" << pt.x << "," << pt.y << "," << pt.z << ")";
            return out;
        }
    };

    /**
     * @brief Tetrahedron class for the 3D barycentric interpolation
     */
    class MeshElement {
    public:
        /**
         * @brief Default constructor
         */
        MeshElement() = delete;

        /**
         * @brief Constructor using a vector to store the index of the nodes and a list for the mesh and electric field
         * points
         * @param dimension Dimension of the nodes
         * @param vertices_tetrahedron List containing 4 mesh node points
         * @param efield_vertices_tetrahedron List containing 4 points with the component of the electric field at the mesh
         * node
         */
        MeshElement(size_t dimension,
                    const std::array<Point, 4>& vertices_tetrahedron,
                    const std::array<Point, 4>& efield_vertices_tetrahedron)
            : dimension_(dimension), vertices_(vertices_tetrahedron), e_field_(efield_vertices_tetrahedron) {
            calculate_volume();
        }

        /**
         * @brief Checks if the tetrahedron is valid for the interpolation
         * @param volume_cut Threshold for the minimum tetrahedron volume. Values <= 0 disable coplanarity checks
         * @param qp Desired point for the interpolation
         */
        bool isValid(double volume_cut, Point& qp) const;

        /**
         * @brief Barycentric interpolation implementation
         * @param qp Point where the interpolation is being done
         */
        Point getObservable(Point& qp) const;

        /**
         * @brief Print tetrahedron information for debugging
         * @return String describing the mesh element
         */
        std::string print(Point& qp) const;

    private:
        /**
         * @brief Get the distance from vertex to a random point
         * @param index Vertex for which the distance will be calculated
         * @param qp Point from which will be calculated the distance
         */
        double get_distance(size_t index, Point& qp) const;

        void calculate_volume();

        double get_sub_volume(size_t index, Point& p) const;

        size_t dimension_{3};
        std::array<Point, 4> vertices_{};
        std::array<Point, 4> e_field_{};

        double volume_{0};
    };

    /**
     * @brief Functor class to be used by the for_each_combination algorithm.
     *
     * It receives pointers to the point and field vectors and its operator() member is called for every combination of
     * results found. It constructs a new MeshElement, checks for its validity and returns true to stop the iteration and
     * false to continue to the next combination of results.
     */
    class Combination {
        const std::vector<Point>* grid_;
        const std::vector<Point>* field_;
        Point reference_;
        Point result_;
        bool valid_{};
        double cut_;

        std::array<Point, 4> grid_elements;
        std::array<Point, 4> field_elements;

    public:
        /**
         * @brief constructor for functor
         * @param  points     Pointer to mesh point vector
         * @param  field      Pointer to field vector
         * @param  q          Reference point to interpolate at
         * @param  volume_cut Volume cut to be used
         */
        explicit Combination(const std::vector<Point>* points,
                             const std::vector<Point>* field,
                             const Point& q,
                             const double volume_cut)
            : grid_(points), field_(field), reference_(q), cut_(volume_cut) {}

        /**
         * @brief Operator called for each permutation
         * @param begin First iterator element of current combination
         * @param end Last iterator element of current combination
         * @return True if valid mesh element was found, stops the loop, or False if not found, continuing loop through
         * combinations.
         */
        template <class It> bool operator()(It begin, It end) {
            // Dimensionality is number of iterator elements minus one:
            size_t dimensions = static_cast<size_t>(end - begin) - 1;
            size_t idx = 0;

            LOG(TRACE) << "Constructing " << dimensions << "D element at " << reference_ << " with mesh points:";
            for(; begin < end; begin++) {
                LOG(TRACE) << "\t\t" << (*grid_)[*begin];
                grid_elements[idx] = (*grid_)[*begin];
                field_elements[idx++] = (*field_)[*begin];
            }

            MeshElement element(dimensions, grid_elements, field_elements);
            valid_ = element.isValid(cut_, reference_);
            if(valid_) {
                LOG(DEBUG) << element.print(reference_);
                result_ = element.getObservable(reference_);
                if(!result_.isFinite()) {
                    LOG(WARNING) << "Interpolated result not a finite number at " << reference_;
                    return false;
                }
            }

            return valid_;
        }

        /**
         * @brief Member to check for validity of returned mesh element:
         * @return True if element is valid and can be used, False if no valid element was found but iteration ended without
         * result
         */
        bool valid() const { return valid_; }

        /**
         * @brief Member to retrieve interpolated result from valid mesh element
         * @return Interpolated result from valid mesh element
         */
        const Point& result() const { return result_; }
    };

} // namespace mesh_converter

#endif // ALLPIX_MESHELEMENT_H
