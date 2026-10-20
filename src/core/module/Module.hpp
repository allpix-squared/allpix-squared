/**
 * @file
 * @brief Base for the module implementation
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_H
#define ALLPIX_MODULE_H

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <TDirectory.h>

#include "ModuleIdentifier.hpp"
#include "core/config/ConfigManager.hpp"
#include "core/config/Configuration.hpp"
#include "core/geometry/Detector.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/histograms/HistogramManager.hpp"
#include "core/log/Logger.hpp"
#include "core/messenger/delegates.h"
#include "core/module/exceptions.h"
#include "core/utils/prng.h"

namespace allpix {
    class Messenger;
    class Event;
    /**
     * @defgroup Modules Modules
     * @brief Collection of modules included in the framework
     */
    /**
     * @brief Base class for all modules
     *
     * The module base is the core of the modular framework. All modules should be descendants of this class. The base class
     * defines the methods the children can implement:
     * - Module::initialize(): for initializing the module at the start
     * - Module::run(Event*): for doing the job of every module for every event
     * - Module::finalize(): for finalizing the module at the end
     *
     * The module class also provides a few utility methods and stores internal data of instantiations. The internal data is
     * used by the ModuleManager and the Messenger to work.
     */
    class Module {
        friend class Event;
        friend class ModuleManager;
        friend class Messenger;
        friend class LocalMessenger;
        friend class HistogramManager;

    public:
        /**
         * @brief Essential virtual destructor.
         *
         * Also removes all delegates linked to this module
         */
        virtual ~Module();

        /// @{
        /**
         * @brief Copying a module is not allowed
         */
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;
        /// @}

        /// @{
        /**
         * @brief Disallow move behaviour (not possible with references)
         */
        Module(Module&&) noexcept = delete;
        Module& operator=(Module&&) noexcept = delete;
        /// @}

        /**
         * @brief Get the detector linked to this module
         * @return Linked detector or a null pointer if this is an unique module
         *
         * Detector modules always have a linked detector and unique modules are guaranteed not to have one.
         */
        virtual std::shared_ptr<Detector> getDetector() const { return nullptr; }

        /**
         * @brief Get the unique name of this module
         * @return Unique name
         * @note Can be used to interact with ROOT objects that require an unique name
         */
        std::string getUniqueName() const;

        /**
         * @brief Get the logger instance of this module
         * @return Reference to the module's logger, valid for the remainder of the program
         */
        Logger& getLogger() const { return logger_; }

        /**
         * @brief Create and return an absolute path to be used for output from a relative path
         * @param pathname Relative path to add after the main output directory
         * @param extension File extension to be added to filename if it doesn't exist yet
         * @param global True if the global output directory should be used instead of the module-specific version
         * @param delete_file True if the created output file should be deleted after testing access
         * @return Canonical path to an output file
         */
        std::string createOutputFile(const std::string& pathname,
                                     const std::string& extension = "",
                                     bool global = false,
                                     bool delete_file = false);

        /**
         * @brief Creates a ThreadedHistogram object through the histogram manager
         * @param args Arguments for histogram constructor
         * @return Shared pointer to generated histogram object
         */
        template <typename T, class... ARGS>
        std::shared_ptr<ThreadedHistogram<T>> CreateHistogram(ARGS&&... args) { // NOLINT
            // Get instance of histogram registry
            return histogram_manager_->registerHistogram<T>(this->getROOTDirectory(), std::forward<ARGS>(args)...);
        }

        /**
         * @brief Creates a ThreadedHistogram object through the histogram manager, as well as a subdirectory for this to be
         * stored in if necessary
         * @param subdirectory Name of the subdirectory the histogram should be stored in
         * @param args Arguments for histogram constructor
         * @return Shared pointer to generated histogram object
         */
        template <typename T, class... ARGS>
        std::shared_ptr<ThreadedHistogram<T>> CreateHistogramSubdirectory(const std::string& subdirectory, // NOLINT
                                                                          ARGS&&... args) {
            // Get instance of histogram registry
            auto* full_directory = allpix::HistogramManager::registerSubdirectory(this->getROOTDirectory(), subdirectory);
            return histogram_manager_->registerHistogram<T>(full_directory, std::forward<ARGS>(args)...);
        }

        /**
         * @brief Get ROOT directory which should be used to output histograms et cetera
         * @return ROOT directory for storage
         */
        TDirectory* getROOTDirectory() const;

        /**
         * @brief Get the config manager object to allow to read the global and other module configurations
         * @return Pointer to the config manager
         */
        ConfigManager* getConfigManager() const;

        /**
         * @brief Returns if multithreading of this module is enabled
         * @return True if multithreading is enabled, false otherwise (the default)
         */
        bool multithreadingEnabled() const { return multithreading_; }

        /**
         * @brief Initialize the module for each thread after the global initialization
         * @note Useful to prepare thread local objects
         *
         * Does nothing if not overloaded.
         */
        virtual void initializeThread() {}

        /**
         * @brief Initialize the module before the event sequence
         *
         * Does nothing if not overloaded.
         */
        virtual void initialize() {}

        /**
         * @brief Execute the function of the module for every event
         * @param event Pointer to the event the module is running
         *
         * Does nothing if not overloaded.
         */
        virtual void run(Event* event) { (void)event; }

        /**
         * @brief Finalize the module after the event sequence for each thread
         * @note Useful to cleanup thread local objects
         *
         * Does nothing if not overloaded.
         */
        virtual void finalizeThread() {}

        /**
         * @brief Finalize the module after the event sequence
         * @note Useful to have before destruction to allow for raising exceptions
         *
         * Does nothing if not overloaded.
         */
        virtual void finalize() {}

    protected:
        /**
         * @brief Base constructor for modules
         * @param config Configuration for this module
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         */
        explicit Module(Configuration& config, Messenger* messenger);
        /**
         * @brief Enable multithreading for this module
         */
        void allow_multithreading() { set_multithreading(true); }

        /**
         * @brief Sets the sequence requirement flag
         */
        void set_sequence_required(bool sequence_requred = true) { sequence_required_ = sequence_requred; }

        /**
         * @brief Get the module configuration for internal use
         * @return Configuration of the module
         */
        Configuration& get_configuration() { return config_; }
        Configuration& config_;
        Logger& logger_;

        Messenger* messenger_{};

    private:
        /**
         * @brief Set the module identifier for internal use
         * @param identifier Identifier of the instantiation
         */
        void set_identifier(ModuleIdentifier identifier) { identifier_ = std::move(identifier); }
        /**
         * @brief Get the module identifier for internal use
         * @return Identifier of the instantiation
         */
        ModuleIdentifier get_identifier() const { return identifier_; }
        ModuleIdentifier identifier_;

        /**
         * @brief Set the output ROOT directory for this module
         * @param directory ROOT directory for storage
         */
        void set_root_directory(TDirectory* directory);
        TDirectory* directory_{nullptr};

        /**
         * @brief Set the link to the config manager
         * @param conf_manager Pointer to the configuration manager holding all relevant configurations
         */
        void set_config_manager(ConfigManager* conf_manager);
        ConfigManager* conf_manager_{nullptr};

        /**
         * @brief Set the link to the histogram manager
         * @param histogram_manager Pointer to the histogram manager holding all histograms
         */
        void set_histogram_manager(HistogramManager* histogram_manager);
        HistogramManager* histogram_manager_{nullptr};

        /**
         * @brief Add a messenger delegate to this instantiation
         * @param messenger Pointer to the messenger responsible for the delegate
         * @param delegate Delegate object
         */
        void add_delegate(Messenger* messenger, BaseDelegate* delegate);

        /**
         * @brief Check if all delegates are satisfied
         * @param messenger Pointer to the messenger we want to check with
         * @param event Pointer to the event we want to check with
         */
        bool check_delegates(Messenger* messenger, Event* event);

        /**
         * @brief Inform the module that a certain event will be skipped
         */
        virtual void skip_event(uint64_t /*unused*/) {}

        std::vector<std::pair<Messenger*, BaseDelegate*>> delegates_;

        /**
         * @brief Sets the multithreading flag
         */
        void set_multithreading(bool multithreading) { multithreading_ = multithreading; }
        bool multithreading_{false};

        /**
         * @brief Checks if this module needs to be executed in correct event sequence
         *
         * @return true if strict event sequence is required and false if any processing order is valid
         */
        bool require_sequence() const { return sequence_required_; }
        bool sequence_required_{false};

        /**
         * @brief Get this module identifier unique name directly from the instance configuration
         * @note the `identifier` key is set by the \ref ConfigManager when adding the instance configuration
         */
        static std::string module_unique_name(const Configuration& config);

        /**
         * @brief Get or create the module logger, configuring the level via the `log_level` config key.
         * @note This method is called by the \ref ModuleManager before constructing the module, so construction itself can
         * be logged through the module logger) and by the \ref Module base constructor.
         *
         * @throws InvalidValueError If `log_level` is set to an invalid value
         */
        static Logger& generate_logger(const Configuration& config);
    };

    class UniqueModule : public Module {
        friend class Event;
        friend class ModuleManager;
        friend class Messenger;

    public:
        /**
         * @brief Base constructor for unique modules
         * @param config Configuration for this module
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        explicit UniqueModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
            : Module(config, messenger), geo_manager_(geo_manager) {};

        /**
         * @brief Get the geometry manager
         * @return Pointer to geometry manager
         */
        GeometryManager* getGeometryManager() const { return geo_manager_; }

    private:
        GeometryManager* geo_manager_{};
    };

    class DetectorModule : public Module {
        friend class Event;
        friend class ModuleManager;
        friend class Messenger;

    public:
        /**
         * @brief Base constructor for detector modules
         * @param config Configuration for this module
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Detector bound to this module
         * @warning Detector modules should not forget to forward their detector to the base constructor. An
         *          \ref InvalidModuleStateException will be raised if the module failed to so.
         */
        explicit DetectorModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
            : Module(config, messenger), detector_(std::move(detector)) {};

        /**
         * @brief Get the detector linked to this module
         * @return Linked detector or a null pointer if this is an unique module
         *
         * Detector modules always have a linked detector and unique modules are guaranteed not to have one.
         */
        std::shared_ptr<Detector> getDetector() const override { return detector_; }

    private:
        std::shared_ptr<Detector> detector_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_H */
