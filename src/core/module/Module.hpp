/**
 * @file
 * @brief Base for the module implementation
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_H
#define ALLPIX_MODULE_H

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <random>
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
     * - Module::init(): for initializing the module at the start
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
         */
        std::shared_ptr<Detector> getDetector() const;

        /**
         * @brief Get the unique name of this module
         * @return Unique name
         * @note Can be used to interact with ROOT objects that require an unique name
         */
        std::string getUniqueName() const;

        /**
         * @brief Create and return an absolute path to be used for output from a relative path
         * @param path Relative path to add after the main output directory
         * @param global True if the global output directory should be used instead of the module-specific version
         * @param delete_file True if the created output file should be deleted after testing access
         * @return Canonical path to an output file
         */
        std::string createOutputFile(const std::string& path, bool global = false, bool delete_file = false);

        /**
         * @brief Get seed to initialize random generators
         * @warning This should be the only method used by modules to seed random numbers to allow reproducing results
         */
        uint64_t getRandomSeed();

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
         * @brief Returns if parallelization of this module is enabled
         * @return True if parallelization is enabled, false otherwise (the default)
         */
        bool canParallelize() const;

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
        virtual void init() {}

        /**
         * @brief Execute the function of the module for every event
         * @param Event Pointer to the event the module is running
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
         * @brief Enable parallelization for this module
         */
        void enable_parallelization();

        /**
         * @brief Get the module configuration for internal use
         * @return Configuration of the module
         */
        Configuration& get_configuration();
        Configuration& config_;

    private:
        /**
         * @brief Set the module identifier for internal use
         * @param identifier Identifier of the instantiation
         */
        void set_identifier(ModuleIdentifier identifier);
        /**
         * @brief Get the module identifier for internal use
         * @return Identifier of the instantiation
         */
        ModuleIdentifier get_identifier() const;
        ModuleIdentifier identifier_;

        /**
         * @brief Set the output ROOT directory for this module
         * @param directory ROOT directory for storage
         */
        void set_ROOT_directory(TDirectory* directory);
        TDirectory* directory_{nullptr};

        /**
         * @brief Set the link to the config manager
         * @param conf_manager ConfigManager holding all relevant configurations
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
         * @param event Number of event skipped
         */
        virtual void skip_event(uint64_t) {}

        std::vector<std::pair<Messenger*, BaseDelegate*>> delegates_;

        /**
         * @brief Set the random number generator for this module
         * @param random_generator Generator to produce random numbers
         */
        void set_random_generator(RandomNumberGenerator* random_generator);
        RandomNumberGenerator* random_generator_{nullptr};

        std::shared_ptr<Detector> detector_;

        /**
         * @brief Sets the parallelize flag
         */
        void set_parallelize(bool parallelize);
        bool parallelize_{false};

        /**
         * @brief Checks if object is instance of BufferedModule class
         */
        virtual bool is_buffered() const { return false; }
    };

    /**
     * @brief A Module that always ensure to execute events in the order of event numbers. It
     * implements buffering out of the box so interested modules can directly use it
     */
    class BufferedModule : public Module {
        friend class Event;
        friend class ModuleManager;
        friend class Messenger;

    public:
        explicit BufferedModule(Configuration& config) : Module(config) {}
        explicit BufferedModule(Configuration& config, std::shared_ptr<Detector> detector)
            : Module(config, std::move(detector)) {}

    protected:
        /**
         * @brief Finalize the module after the event sequence
         */
        virtual void finalize_buffer();

        /**
         * @brief Run the event in the order of increasing event number
         * @param event The event to run
         * @return number of events currently in the buffer
         *
         * Execute the event if it is in the correct order, otherwise buffer the event and execute it later
         */
        virtual size_t run_in_order(std::shared_ptr<Event> event);

    private:
        /**
         * @brief Returns the next event number to run
         * @param event Current event number to check from
         * @return Event number for next event to run
         */
        uint64_t get_next_event(uint64_t);

        /**
         * @brief Inform the module that a certain event will be skipped
         * @param event Number of event skipped
         */
        void skip_event(uint64_t) override;

        /**
         * @brief Flush the buffered events
         */
        void flush_buffered_events();

        /**
         * @brief Checks if object is instance of BufferedModule class
         */
        bool is_buffered() const override { return true; }

        // The buffer that holds out of order events
        std::map<uint64_t, std::shared_ptr<Event>> buffered_events_;

        // Mutex used to guard access to \ref buffered_events_
        std::mutex buffer_mutex_;

        // Mutex used to allow only one thread to run the writer module
        std::mutex writer_mutex_;

        std::condition_variable cond_var_;

        // Set of event numbers that was skipped because not all messages were present
        std::set<uint64_t> skipped_events_;

        // Mutex to guard access to \ref skipped_events_mutex_
        std::mutex skipped_events_mutex_;

        // The expected in order event to write
        std::atomic_uint64_t next_event_to_write_{1};

        // Maximum buffer size to store events
        static size_t max_buffer_size_;

        /**
         * @brief Sets the buffer size.
         * @param size The new buffer size
         */
        static void set_max_buffer_size(size_t size) { max_buffer_size_ = size; }
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_H */
