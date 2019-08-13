/**
 * @file
 * @brief Creation and execution of an event
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_EVENT_H
#define ALLPIX_MODULE_EVENT_H

#include <atomic>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <random>
#include <vector>

namespace allpix {
    class Module;
    class Messenger;
    class BaseMessage;
    class LocalMessenger;
    using ModuleList = std::list<std::shared_ptr<Module>>;

    /**
     * @brief Wrapper for the objects shared and used by all event objects.
     */
    struct event_context {
        event_context(Messenger& messenger,
                      ModuleList modules,
                      std::map<Module*, long double>& module_execution_time,
                      std::atomic<bool>& terminate)
            : messenger_(messenger), modules_(std::move(modules)), module_execution_time_(module_execution_time),
              terminate_(terminate) {}

        // Global messenger object
        Messenger& messenger_;

        // List of modules that constitutes this event
        ModuleList modules_;

        // Storage of module run-time statistics
        std::map<Module*, long double>& module_execution_time_;

        // Simulation-global termination flag
        std::atomic<bool>& terminate_;
    };

    /**
     * @ingroup Managers
     * @brief Manager responsible for the execution of a single event
     *
     * Executes a single event, storing messages independent from other events.
     * Exposes an API for usage by the modules' \ref Module::run() "run functions".
     */
    class Event {
        friend class ModuleManager;

    public:
        /// @{
        /**
         * @brief Copying an event not allowed
         */
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
        /**
         * @brief Disallow move because of mutexes and atomics (?)
         */
        Event(Event&&) = delete;
        Event& operator=(Event&&) = delete;
        /// @}

        /**
         * @brief Unique identifier of this event
         */
        const unsigned int number;

        /**
         * @brief Returns the messenger instance used within this event
         */
        Messenger* getMessenger() const;

        /**
         * @brief Access the random engine of this event
         * @return Reference to this event's random engine
         */
        std::mt19937_64& getRandomEngine() { return random_engine_; }

        /**
         * @brief Advances the random engine's state one step
         * @return The generated value
         */
        uint64_t getRandomNumber() { return random_engine_(); }

    private:
        /**
         * @brief Construct an Event
         * @param event_num The unique event identifier
         * @param random_engine Random generator for this event
         */
        explicit Event(const unsigned int event_num, std::mt19937_64& random_engine);

        /**
         * @brief Use default destructor
         */
        ~Event() = default;

        /**
         * @brief Sequentially run the event's remaining modules
         *
         * May be called in a spawned thread.
         */
        void run();

        /**
         * Handle the execution of a single module
         * @param module The module to execute
         */
        void run(std::shared_ptr<Module>& module);

        /**
         * @brief Sets the common objects used by all events.
         */
        static void setEventContext(event_context* context) { context_ = context; }

        // Mutex needed to record execution time since atomics doesn't fully support floating points
        static std::mutex stats_mutex_;

        // The random number engine associated with this event
        std::mt19937_64& random_engine_;

        // Shared objects between all events
        static event_context* context_;

        LocalMessenger* local_messenger_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_EVENT_H */
