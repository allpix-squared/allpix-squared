/**
 * @file
 * @brief Keeping track of the global geometry of independent detectors
 * @copyright MIT License
 */

#ifndef ALLPIX_GEOMETRY_MANAGER_H
#define ALLPIX_GEOMETRY_MANAGER_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Detector.hpp"

namespace allpix {

    /**
     * @ingroup Managers
     * @brief Manager responsible for the global geometry
     *
     * The framework defines the geometry as a set of independent instances of a \ref Detector "detector". Each independent
     * detector has a \ref DetectorModel "detector model".
     */
    class GeometryManager {
    public:
        /**
         * @brief Construct the geometry manager
         */
        GeometryManager();
        /**
         * @brief Use default destructor
         */
        ~GeometryManager() = default;

        /// @{
        /**
         * @brief Copying the manager is not allowed
         */
        GeometryManager(const GeometryManager&) = delete;
        GeometryManager& operator=(const GeometryManager&) = delete;
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        GeometryManager(GeometryManager&&) noexcept = default;
        GeometryManager& operator=(GeometryManager&&) noexcept = default;
        /// @}

        /**
         * @brief Add a detector to the global geometry
         * @param detector Pointer to the detector to add to the geometry
         */
        void addDetector(std::shared_ptr<Detector> detector);

        /**
         * @brief Get all detectors in the geometry
         * @returns List of all detectors
         */
        std::vector<std::shared_ptr<Detector>> getDetectors() const;

        /**
         * @brief Get a detector by its name
         * @param name Name of the detector to search for
         * @returns Return the detector if it exists (an error is raised if it does not)
         * @warning This method should not be used to check if a detector name exists
         */
        // TODO [doc] Add a has detector
        std::shared_ptr<Detector> getDetector(const std::string& name) const;

        /**
         * @brief Get all detectors in the geometry of a particular model type
         * @param type Type of the detector model to search for
         * @returns List of all detectors of a particular type (an error is raised if none exist)
         * @warning This method should not be used to check if an instantiation of a model exists
         */
        // TODO [doc] Add a type exists in geometry?
        std::vector<std::shared_ptr<Detector>> getDetectorsByType(const std::string& type) const;

    private:
        std::vector<std::shared_ptr<Detector>> detectors_;

        std::set<std::string> detector_names_;
    };
} // namespace allpix

#endif /* ALLPIX_GEOMETRY_MANAGER_H */
