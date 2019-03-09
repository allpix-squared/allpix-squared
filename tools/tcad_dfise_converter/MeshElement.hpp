#include <Eigen/Eigen>
#include <array>
#include <utility>

#include "core/utils/log.h"

#include "DFISEParser.hpp"
#include "octree/Octree.hpp"

namespace mesh_converter {
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
         * @param index List of indices of the points for debugging purposes
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
         * @param volume_cut Threshold for the minimum tetrahedron volume
         * @param qp Desired point for the interpolation
         */
        bool validElement(double volume_cut, Point& qp) const;

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

    class f {
        Point* grid_;
        Point* field_;
        Point reference_;
        Point result_;
        bool valid_{};
        double cut_;

        std::array<Point, 4> grid_elements;
        std::array<Point, 4> field_elements;

    public:
        explicit f(Point* points, Point* field, const Point& q, const double volume_cut)
            : grid_(points), field_(field), reference_(q), cut_(volume_cut) {}

        // called for each permutation
        template <class It> bool operator()(It begin, It end) {
            // Dimensionality is number of iterator elements minus one:
            size_t dimensions = static_cast<size_t>(end - begin) - 1;
            size_t idx = 0;
            for(; begin < end; begin++) {
                grid_elements[idx] = *(grid_ + (*begin));
                field_elements[idx++] = *(field_ + (*begin));
            }

            LOG(TRACE) << "Constructing element with dim " << dimensions << " at " << reference_;
            MeshElement element(dimensions, grid_elements, field_elements);
            valid_ = element.validElement(cut_, reference_);
            if(valid_) {
                LOG(DEBUG) << element.print(reference_);
                result_ = element.getObservable(reference_);
            }

            return valid_; // Don't break out of the loop
        }

        bool valid() const { return valid_; }
        Point result() const { return result_; }
    };

} // namespace mesh_converter
