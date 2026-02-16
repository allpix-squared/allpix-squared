/**
 * @file
 * @brief Creation and execution of an event
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_EVENT_H
#define ALLPIX_MODULE_EVENT_H

#include <atomic>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "core/utils/prng.h"

namespace allpix {
    class Module;
    class Messenger;
    class BaseMessage;
    class LocalMessenger;

    /**
     * @brief Holds the data required for running an event
     */
    class Event {
        friend class ModuleManager;
        friend class Messenger;
        friend class SequentialModule;

    public:
        /**
         * @brief Construct an Event
         * @param messenger Messenger responsible for handling message transmission for this event
         * @param event_num The unique event identifier
         * @param seed Random generator seed for this event
         */
        explicit Event(Messenger& messenger, uint64_t event_num, uint64_t seed);
        /**
         * @brief Use default destructor
         */
        ~Event() = default;

        /// @{
        /**
         * @brief Disallow move because of mutexes and atomics
         */
        Event(Event&&) = delete;
        Event& operator=(Event&&) = delete;
        /// @}

        /// @{
        /**
         * @brief Copying a module is not allowed
         */
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
        /// @}

        /**
         * @brief Unique identifier of this event
         */
        const uint64_t number;

        /**
         * @brief Access the random engine of this event
         * @return Reference to this event's random engine
         */
        RandomNumberGenerator& getRandomEngine();

        /**
         * @brief Advances the random engine's state by one step
         * @return The generated value
         */
        uint64_t getRandomNumber() { return getRandomEngine()(); }

        /**
         * @brief Returns the current seed for the ranom number generator
         * @return The random seed of the current event
         */
        uint64_t getSeed() const { return seed_; }

    private:
        /**
         * @brief Sets the random engine and seed it to be used by this event
         * @param random_engine Pointer to RNG for this event
         */
        void set_and_seed_random_engine(RandomNumberGenerator* random_engine);

        /**
         * @brief Store the state of the PRNG
         */
        void store_random_engine_state();

        /**
         * @brief Restore the state of the PRNG from the stored internal state
         */
        void restore_random_engine_state();

        // The random number engine associated with this event
        RandomNumberGenerator* random_engine_{nullptr};

        // Seed for random number generator
        uint64_t seed_;

        // State of the random number generator
        std::stringstream state_;

        /**
         * @brief Returns a pointer to the event local messenger
         */
        LocalMessenger* get_local_messenger() const;

        // Local messenger used to dispatch messages in this event
        std::unique_ptr<LocalMessenger> local_messenger_;

        // Mutex for execution time
        static std::mutex stats_mutex_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_EVENT_H */
