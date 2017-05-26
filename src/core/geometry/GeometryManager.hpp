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
#include "DetectorModel.hpp"
#include "core/config/ConfigReader.hpp"

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
         * @param reader Reader for the file containing the detector configuration
         */
        GeometryManager(ConfigReader reader);
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
         * @brief Loads the geometry from the reader
         * @warning Has to be the first function called after the constructor
         */
        void load();

        /**
         * @brief Add a detector model and apply it to the registered detectors
         * @param model Pointer to the detector model
         * @warning Can only be used as long as the geometry is still open
         */
        void addModel(std::shared_ptr<DetectorModel> model);

        /**
         * @brief Check if a detector model exists
         * @return True if the model is part of the geometry, false otherwise
         */
        bool hasModel(const std::string& name) const;

        /**
         * @brief Get all added detector models
         * @returns List of all models
         * @warning The models returned might not be used in the geometry
         */
        std::vector<std::shared_ptr<DetectorModel>> getModels() const;

        /**
         * @brief Get a detector model by its name
         * @param name Name of the model to search for
         * @returns Return the model if it exists (an error is raised if it does not)
         * @warning This method should not be used to check if a model exists
         */
        // TODO [doc] Add a has model
        std::shared_ptr<DetectorModel> getModel(const std::string& name) const;

        /**
         * @brief Add a detector to the global geometry
         * @param detector Pointer to the detector to add to the geometry
         * @warning Can only be used as long as the geometry is still open
         */
        // TODO [doc] Remove this method
        void addDetector(std::shared_ptr<Detector> detector);

        /**
         * @brief Check if a detector exists
         * @return True if the detector is part of the geometry, false otherwise
         */
        bool hasDetector(const std::string& name) const;

        /**
         * @brief Get all detectors in the geometry
         * @returns List of all detectors
         * @note Closes the geometry if it not has been closed yet
         */
        std::vector<std::shared_ptr<Detector>> getDetectors();

        /**
         * @brief Get a detector by its name
         * @param name Name of the detector to search for
         * @returns Return the detector if it exists (an error is raised if it does not)
         * @warning The method \ref GeometryManager::hasDetector should be used to check for existence
         * @note Closes the geometry if it has not been closed yet
         */
        std::shared_ptr<Detector> getDetector(const std::string& name);

        /**
         * @brief Get all detectors in the geometry of a particular model type
         * @param type Type of the detector model to search for
         * @returns List of all detectors of a particular type (an error is raised if none exist)
         * @warning This method should not be used to check if an instantiation of a model exists
         * @note Closes the geometry if it has not been closed yet
         */
        std::vector<std::shared_ptr<Detector>> getDetectorsByType(const std::string& type);

    private:
        /**
         * @brief Close the geometry after which changes to the detector geometry cannot be made anymore
         */
        void close_geometry();
        bool closed_;

        ConfigReader reader_;

        std::vector<std::shared_ptr<DetectorModel>> models_;
        std::set<std::string> model_names_;

        std::map<Detector*, std::string> nonresolved_models_;
        std::vector<std::shared_ptr<Detector>> detectors_;
        std::set<std::string> detector_names_;
    };
} // namespace allpix

#endif /* ALLPIX_GEOMETRY_MANAGER_H */
