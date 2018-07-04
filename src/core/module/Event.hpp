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

    class Event {
    public:
        /**
         * @brief Construct Event
         * @param modules The modules that constitutes the event
         * @param event_num The event identifier
         * @param terminate TODO
         * @param module_execution_time_ TODO
         */
        explicit Event(ModuleList modules,
                       const unsigned int event_num,
                       std::atomic<bool>& terminate,
                       std::map<Module*, long double>& module_execution_time_,
                       Messenger* messenger);
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
         * @brief Run the event, updating the recorded module execution time on the side
         * @param number_of_events The total number of events in the simulation
         * @param module_execution_time The map that stores module execution times
         * @warning Should be called after the \ref Event::init "init function"
         */
        void run(const unsigned int number_of_events);

        /**
         * @brief Finalize the event
         * @warning Should be called after the \ref Event::run "run function"
         */
        void finalize();

    private:
        ModuleList modules_;
        MessageStorage message_storage_;
        const unsigned int event_num_;

        // XXX: cannot be moved
        std::atomic<bool>& terminate_;

        // XXX: must this be mutex protected?
        std::map<Module*, long double>& module_execution_time_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_EVENT_H */
