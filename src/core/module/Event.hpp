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
#include "MessageStorage.hpp"
#include "ModuleManager.hpp"

#include "../messenger/Messenger.hpp"

namespace allpix {
    struct IOLock {
        std::mutex mutex;
        std::condition_variable condition;
        std::atomic<unsigned int> current_event{1};
    };

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
         * @brief Fetches a message
         */
        template <typename T> std::shared_ptr<T> fetchMessage();

        /**
         * @brief Fetches a vector of messages
         */
        template <typename T> std::vector<std::shared_ptr<T>> fetchMultiMessage();

        std::mt19937_64& getRandomEngine() { return random_generator_; }

        uint64_t getRandomNumber() { return random_generator_(); }

    private:
        /**
         * @brief Construct Event
         * @param modules The modules that constitutes the event
         * @param event_num The event identifier
         * @param terminate TODO
         * @param module_execution_time TODO
         * @param seeder TODO
         * @param io_mutex TODO
         * @param io_condition TODO
         */
        explicit Event(ModuleList modules,
                       const unsigned int event_num,
                       std::atomic<bool>& terminate,
                       std::map<Module*, long double>& module_execution_time,
                       Messenger* messenger,
                       std::mt19937_64& seeder,
                       IOLock& reader_lock,
                       IOLock& writer_lock);
        /**
         * @brief Use default destructor
         */
        /* ~Event() = default; */

        // TODO: fix this
        /// @{
        /**
         * @brief Copying an event not allowed
         */
        /* Event(const Event&) = default; */
        /* Event& operator=(const Event&) = default; */
        /**
         * @brief Moving an event is allowed
         */
        /* Event(Event&&) = default; */
        /* Event& operator=(Event&&) = default; */
        /// @}

        /**
         * @brief Inialize the event
         */
        void init();

        /**
         * @brief Run the event
         * @warning Should be called after the \ref Event::init "init function"
         */
        void run();
        void run(std::shared_ptr<Module>& module);

        bool handle_iomodule(const std::shared_ptr<Module>& module);

        /**
         * @brief Finalize the event
         * @warning Should be called after the \ref Event::run "run function"
         */
        void finalize();

        ModuleList modules_;
        MessageStorage message_storage_;

        // XXX: cannot be moved
        std::atomic<bool>& terminate_;

        // XXX: must this be mutex protected?
        std::map<Module*, long double>& module_execution_time_;

        std::mt19937_64 random_generator_;

        // For Readers/Writers execution
        IOLock& reader_lock_;
        IOLock& writer_lock_;
        bool previous_was_reader_{false};
    };

} // namespace allpix

// Include template members
#include "Event.tpp"

#endif /* ALLPIX_MODULE_EVENT_H */
