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

#include "ModuleManager.hpp"

#include "../messenger/Messenger.hpp"

namespace allpix {

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
         * @brief Dispatches a message to subscribing modules
         * @param message Pointer to the message to dispatch
         * @param name Optional message name (defaults to - indicating that it should dispatch to the module output
         * parameter)
         */
        template <typename T> void dispatchMessage(std::shared_ptr<T> message, const std::string& name = "-") {
            return context_->messenger_.dispatchMessage(current_module_, message, name);
        }

        /**
         * @brief Fetches a single message of specified type meant for the calling module
         * @return Shared pointer to message
         */
        template <typename T> std::shared_ptr<T> fetchMessage() {
            return context_->messenger_.fetchMessage<T>(current_module_);
        }

        /**
         * @brief Fetches multiple messages of specified type meant for the calling module
         * @return Vector of shared pointers to messages
         */
        template <typename T> std::vector<std::shared_ptr<T>> fetchMultiMessage() {
            return context_->messenger_.fetchMultiMessage<T>(current_module_);
        }

        /**
         * @brief Fetches filtered messages meant for the calling module
         * @return Vector of pairs containing shared pointer to and name of message
         */
        std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> fetchFilteredMessages() {
            return context_->messenger_.fetchFilteredMessages(current_module_);
        }

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
         * @brief Helper struct used to run modules requiring I/O operations in order of event number
         */
        struct IOOrderLock { // NOLINT
            std::mutex mutex;
            std::condition_variable condition;
            std::atomic<unsigned int> current_event{1};

            /**
             * @brief Increment the current event and notify all waiting threads
             */
            void next() {
                current_event++;
                condition.notify_all();
            }
        };

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
         * @brief Handles the execution of modules requiring I/O operations
         * @param module The module to execute
         * @warning This function should be called for every module in \ref Event::modules_
         *
         * If modules that require I/O operations are passed to this function from multiple threads, each thread will exit
         * this function in order of increasgin event number. This ensures that readers and writers run in the same order
         * between two same-configuration simulations.
         */
        void handle_iomodule(const std::shared_ptr<Module>& module);

        /**
         * @brief Sets the common objects used by all events.
         */
        static void setEventContext(event_context* context) { context_ = context; }

        static std::mutex stats_mutex_;

        // For executing readers/writers in order of event number
        static IOOrderLock reader_lock_;
        static IOOrderLock writer_lock_;
        bool previous_was_reader_{false};

        std::mt19937_64& random_engine_;

        Module* current_module_;

        // Shared objects between all events
        static event_context* context_;

#ifndef NDEBUG
        static std::set<unsigned int> unique_ids_;
#endif
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_EVENT_H */
