#include <Eigen/Eigen>
#include <utility>
#include <vector>

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
         * @brief Constructor with vector containing the vertices points
         * @param vertices_tetrahedron List containing 4 mesh node points
         */
        explicit MeshElement(std::vector<Point> vertices_tetrahedron) : vertices_(std::move(vertices_tetrahedron)) {
            calculate_volume();
        }

        /**
         * @brief Constructor using a vector to store the index of the nodes and a list for the mesh and electric field
         * points
         * @param dimension Dimension of the nodes
         * @param index List of indices of the points for debugging purposes
         * @param vertices_tetrahedron List containing 4 mesh node points
         * @param efield_vertices_tetrahedron List containing 4 points with the component of the electric field at the mesh
         * node
         */
        MeshElement(int dimension,
                    std::vector<size_t> index,
                    std::vector<Point> vertices_tetrahedron,
                    std::vector<Point> efield_vertices_tetrahedron)
            : dimension_(dimension), indices_(std::move(index)), vertices_(std::move(vertices_tetrahedron)),
              e_field_(std::move(efield_vertices_tetrahedron)) {
            calculate_volume();
        }

        /**
         * @brief Get dimension of the element
         */
        int getDimension() const;

        /**
         * @brief Set dimension of the element
         */
        void setDimension(int dimension);

        /**
         * @brief Get the volume of the tetrahedron
         */
        double getVolume() const;

        /**
         * @brief Get the distance from vertex to a random point
         * @param index Vertex for which the distance will be calculated
         * @param qp Point from which will be calculated the distance
         */
        double getDistance(size_t index, Point& qp) const;

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
        std::string printElement(Point& qp) const;

    private:
        void calculate_volume();

        int dimension_{3};
        std::vector<size_t> indices_{};
        std::vector<Point> vertices_{};
        std::vector<Point> e_field_{};

        double volume_{0};
    };
} // namespace mesh_converter
