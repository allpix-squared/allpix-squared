/**
 * @file
 * @brief Wrapper class around an event
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_EVENT_H
#define ALLPIX_MODULE_EVENT_H

// XXX: merely for ModuleList
#include "ModuleManager.hpp"

#include "../messenger/Messenger.hpp"

namespace allpix {

    /**
     * @ingroup Managers
     * @brief Manager responsible for the execution of a single event
     *
     * Executes a single event, storing messages independent from other events.
     * Exposes an API for usage by the modules' \ref Module::run "run functions".
     */
    class Event {
        friend class ModuleManager;

    public:
        /**
         * @brief Identifier of this event
         */
        const unsigned int number;

        /**
         * @brief Dispatches a message
         * @param message Pointer to the message to dispatch
         * @param name Optional message name (defaults to - indicating that it should dispatch to the module output
         * parameter)
         */
        template <typename T> void dispatchMessage(std::shared_ptr<T> message, const std::string& name = "-");

        /**
         * @brief Fetches a single message of specified type
         * @return Shared pointer to message
         */
        template <typename T> std::shared_ptr<T> fetchMessage();

        /**
         * @brief Fetches multiple messages of specified type
         * @return Vector of shared pointers to messages
         */
        template <typename T> std::vector<std::shared_ptr<T>> fetchMultiMessage();

        /**
         * @brief Fetches filtered messages
         * @return Vector of pairs containing shared pointer to and name of message
         */
        std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> fetchFilteredMessages();

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
         * @brief Helper struct used to read from and writer to files in order of event number
         */
        struct IOOrderLock {
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
         * @brief Construct Event
         * @param modules The modules that constitutes the event
         * @param event_num The event identifier
         * @param terminate Reference to simulation-global termination flag
         * @param module_execution_time Map to store module statistics
         * @param seeder Seeder to seed the random engine
         */
        explicit Event(ModuleList modules,
                       const unsigned int event_num,
                       std::atomic<bool>& terminate,
                       std::map<Module*, long double>& module_execution_time,
                       Messenger* messenger,
                       std::mt19937_64& seeder);

        /**
         * @brief Use default destructor
         */
        ~Event() = default;

        // TODO: delete copy constructor
        /// @{
        /**
         * @brief Copying an event not allowed
         */
        Event(const Event&) = default;
        Event& operator=(const Event&) = default;
        /**
         * @brief Moving an event is allowed
         */
        Event(Event&&) = default;
        Event& operator=(Event&&) = default;
        /// @}

        /**
         * @brief Run all Geant4 module, initializing the event
         */
        void run_geant4();

        /**
         * @brief Run the event
         * @warning Should be called after the \ref Event::run_geant4 "init function"
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
         */
        bool handle_iomodule(const std::shared_ptr<Module>& module);

        /**
         * @brief Changes the current context to the given module, used for message handling
         * @param module The new context
         * @warning This function should be called before calling the same module's \ref Module::run "run function"
         */
        Event* with_context(const std::shared_ptr<Module>& module) {
            current_module_ = module.get();
            return this;
        }

        /**
         * @brief Finalize the event
         * @warning Should be called after the \ref Event::run "run function"
         */
        void finalize();

        void dispatch_message(Module* source, std::shared_ptr<BaseMessage> message, std::string name);
        bool dispatch_message(Module* source,
                              const std::shared_ptr<BaseMessage>& message,
                              const std::string& name,
                              const std::string& id);
        /**
         * @brief Check if a module is satisfied for running (all required messages received)
         * @return True if satisfied, false otherwise
         */
        bool is_satisfied(Module* module) const;

        // List of modules that constitutes this event
        ModuleList modules_;

        // Simulation-global termination flag
        std::atomic<bool>& terminate_;

        // Storage of module run-time statistics
        static std::mutex stats_mutex_;
        std::map<Module*, long double>& module_execution_time_;

        // Random engine accessed by all modules in this event
        std::mt19937_64 random_engine_;

        // For execution of readers/writers
        static IOOrderLock reader_lock_;
        static IOOrderLock writer_lock_;
        bool previous_was_reader_{false};

        // For module messages
        DelegateMap& delegates_;
        std::map<std::string, DelegateTypes> messages_;
        std::map<std::string, bool> satisfied_modules_;
        Module* current_module_;
        std::vector<std::shared_ptr<BaseMessage>> sent_messages_;
    };

} // namespace allpix

// Include template members
#include "Event.tpp"

#endif /* ALLPIX_MODULE_EVENT_H */
