#include <Eigen/Eigen>
#include <utility>
#include <vector>

#include "DFISEParser.hpp"
#include "Octree.hpp"

namespace mesh_converter {
    /**
     * @brief Tetrahedron class for the 3D barycentric interpolation
     */
    class MeshElement {
    public:
        /**
         * @brief Default constructor
         */
        explicit MeshElement() = default;

        /**
         * @brief Constructor with vector containing the vertices points
         * @param vertices_tetrahedron List containing 4 mesh node points
         */
        explicit MeshElement(std::vector<Point> vertices_tetrahedron) : vertices_(std::move(vertices_tetrahedron)) {}

        /**
         * @brief Constructor with list for the mesh and the electric field points
         * @param vertices_tetrahedron List containing 4 mesh node points
         * @param efield_vertices_tetrahedron List containing 4 points with the component of the electric field at the mesh
         * node
         */
        MeshElement(std::vector<Point> vertices_tetrahedron, std::vector<Point> efield_vertices_tetrahedron)
            : vertices_(std::move(vertices_tetrahedron)), e_field_(std::move(efield_vertices_tetrahedron)) {}

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
              e_field_(std::move(efield_vertices_tetrahedron)) {}

        /**
         * @brief Setting vertices for the default tetrahedron constructor
         * @param new_vertices std::vector<Points> containing 4 mesh node Points
         */
        void setVertices(std::vector<Point>& new_vertices);

        /**
         * @brief Set individual vertices of the tetrahedron
         * @param index Vertex to be set
         * @param new_vertice New tetrahedron Point
         */
        void setVertex(size_t index, Point& new_vertice);

        /**
         * @brief Get tetrahedron vertice defined by the index
         * @param index Vertex to be returned
         */
        Point getVertex(size_t index);

        /**
         * @brief Set the electric field on the tetrahedron vertices
         * @param new_e_field New std::vector<Point> to be assigned to the tetrahedron
         */
        void setVerticesField(std::vector<Point>& new_e_field);

        /**
         * @brief Set the electric field from individual vertices of the tetrahedron
         * @param index Vertex to be set
         * @param new_e_field New electric field for the tetrahedron vertex
         */
        void setVertexField(size_t index, Point& new_e_field);

        /**
         * @brief Get the point with the components of the electric field for a individual vertex
         * @param index Vertex to return electric field Point
         */
        Point getVertexProperty(size_t index);

        /**
         * @brief Get dimension of the element
         */
        int getDimension();

        /**
         * @brief Set dimension of the element
         */
        void setDimension(int dimension);

        /**
         * @brief Get the volume of the tetrahedron
         */
        double getVolume();

        /**
         * @brief Get the distance from vertex to a random point
         * @param index Vertex for which the distance will be calculated
         * @param qp Point from which will be calculated the distance
         */
        double getDistance(size_t index, Point& qp);

        /**
         * @brief Checks if the tetrahedron is valid for the interpolation
         * @param volume_cut Threshold for the minimum tetrahedron volume
         * @param qp Desired point for the interpolation
         */
        bool validElement(double volume_cut, Point& qp);

        /**
         * @brief Barycentric interpolation implementation
         * @param qp Point where the interpolation is being done
         */
        Point getObservable(Point& qp);

        /**
         * @brief Print tetrahedron information for debugging
         */
        void printElement(Point& qp);

    private:
        int dimension_{3};
        std::vector<size_t> indices_{};
        std::vector<Point> vertices_{};
        std::vector<Point> e_field_{};
    };
} // namespace mesh_converter
