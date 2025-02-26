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

    public:
        /**
         * @brief Base constructor for unique modules
         * @param config Configuration for this module
         */
        explicit Module(Configuration& config);
        /**
         * @brief Base constructor for detector modules
         * @param config Configuration for this module
         * @param detector Detector bound to this module
         * @warning Detector modules should not forget to forward their detector to the base constructor. An
         *          \ref InvalidModuleStateException will be raised if the module failed to so.
         */
        explicit Module(Configuration& config, std::shared_ptr<Detector> detector);
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
        std::shared_ptr<Detector> getDetector() const { return detector_; }

        /**
         * @brief Get the unique name of this module
         * @return Unique name
         * @note Can be used to interact with ROOT objects that require an unique name
         */
        std::string getUniqueName() const;

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
         * @brief Enable multithreading for this module
         */
        void allow_multithreading() { set_multithreading(true); }

        /**
         * @brief Get the module configuration for internal use
         * @return Configuration of the module
         */
        Configuration& get_configuration() { return config_; }
        Configuration& config_;

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
        void set_ROOT_directory(TDirectory* directory);
        TDirectory* directory_{nullptr};

        /**
         * @brief Set the link to the config manager
         * @param config Pointer to the configuration manager holding all relevant configurations
         */
        void set_config_manager(ConfigManager* config);
        ConfigManager* conf_manager_{nullptr};

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
        virtual void skip_event(uint64_t) {}

        std::vector<std::pair<Messenger*, BaseDelegate*>> delegates_;

        std::shared_ptr<Detector> detector_;

        /**
         * @brief Sets the multithreading flag
         */
        void set_multithreading(bool multithreading) { multithreading_ = multithreading; }
        bool multithreading_{false};

        /**
         * @brief Checks if object is instance of SequentialModule class
         */
        virtual bool require_sequence() const { return false; }
    };

    /**
     * @brief A Module that always ensure to execute events in the order of event numbers. It
     * implements buffering out of the box so interested modules can directly use it
     */
    class SequentialModule : public Module {
        friend class Event;
        friend class ModuleManager;
        friend class Messenger;

    public:
        explicit SequentialModule(Configuration& config) : Module(config) {}
        explicit SequentialModule(Configuration& config, std::shared_ptr<Detector> detector)
            : Module(config, std::move(detector)) {}

    protected:
        /**
         * @brief Release strict sequence processing requirement
         */
        void waive_sequence_requirement(bool waive = true);

    private:
        /**
         * @brief Checks if this module needs to be executed in correct event sequence
         *
         * @return true if strict event sequence is required and false if any processing order is valid
         */
        bool require_sequence() const override { return sequence_required_; }
        bool sequence_required_{true};
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_H */
