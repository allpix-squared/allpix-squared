#include <Eigen/Eigen>
#include <iostream>
#include <utility>
#include <vector>

#include "Octree.hpp"
#include "read_dfise.h"

#include "../../src/core/utils/log.h"

// Interrupt handler
void interrupt_handler(int);

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
         * @brief Constructor with vector containing the vertices Points
         * @param vertices_tetrahedron std::vector<Points> containing 4 mesh node Points
         */
        explicit MeshElement(std::vector<Point> vertices_tetrahedron) : vertices(std::move(vertices_tetrahedron)) {}

        /**
         * @brief Explicit constructor with the mesh Points
         * @param p{i} Points from the mesh
         */
        MeshElement(Point& v1, Point& v2, Point& v3, Point& v4) : MeshElement(std::vector<Point>({v1, v2, v3, v4})) {}

        /**
         * @brief Explicit constructor with the mesh Points and the electric field Points
         * @param p{i} Points from the mesh
         * @param f{i} Points with the electric field components from the mesh node
         */
        MeshElement(Point& v1, Point& f1, Point& v2, Point& f2, Point& v3, Point& f3, Point& v4, Point& f4)
            : MeshElement(std::vector<Point>({v1, v2, v3, v4}), std::vector<Point>({f1, f2, f3, f4})) {}

        /**
         * @brief Constructor using std::vector<Points> for the mesh and the electric field Points
         * @param vertices_tetrahedron std::vector<Points> containing 4 mesh node Points
         * @param efield_vertices_tetrahedron std::vector<Points> containing 4 Points with the component of the electric
         * field at
         * the mesh node
         */
        MeshElement(std::vector<Point> vertices_tetrahedron, std::vector<Point> efield_vertices_tetrahedron)
            : vertices(std::move(vertices_tetrahedron)), e_field(std::move(efield_vertices_tetrahedron)) {}

        /**
         * @brief Constructor using a vector to store the index of the noded and a std::vector<Points> for the mesh and
         * electric
         * field Points
         * @param vertices_tetrahedron std::vector<Points> containing 4 mesh node Points
         * @param efield_vertices_tetrahedron std::vector<Points> containing 4 Points with the component of the electric
         * field at
         * the mesh node
         */
        MeshElement(int dimension,
                    std::vector<size_t> index,
                    std::vector<Point> vertices_tetrahedron,
                    std::vector<Point> efield_vertices_tetrahedron)
            : _dimension(dimension), index_vec(std::move(index)), vertices(std::move(vertices_tetrahedron)),
              e_field(std::move(efield_vertices_tetrahedron)) {}

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
         * @brief Set the electric field fro individual vertices of the tetrahedron
         * @param index Vertex to be set
         * @param new_e_field New electric field for the tetrahedron vertex
         */
        void setVertexField(size_t index, Point& new_e_field);

        /**
         * @brief Get the Point with the components of the electric field for a individual vertex
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
         * @brief Get the distance from vertex i to point a random point
         * @param index Vertex for which the distance will be calculated
         * @param qp Point from which will be calculated the distance
         */
        double getDistance(size_t index, Point& qp);

        /**
         * @brief Checks if the tetrahedron is valid for the interpolation
         * @param volume_cut Threshold for the minimum tetrahedron volume
         * @param qp Desired Point for the interpolation
         */
        bool validElement(double volume_cut, Point& qp);

        /**
         * @brief Barycentric interpolation implementation
         * @param qp Point where the interpolation is being done
         */
        Point getObservable(Point& qp);

        /**
         * @brief Print tetrahedron created for debugging
         */
        void printElement(Point& qp);

    private:
        int _dimension{3};
        std::vector<size_t> index_vec{};
        std::vector<Point> vertices{};
        std::vector<Point> e_field{};
    };
} // namespace mesh_converter
