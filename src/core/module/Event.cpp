/**
 * @file
 * @brief Implementation of event
 *
 * @copyright Copyright (c) 2018-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Event.hpp"

#include <algorithm>
#include <chrono>
#include <list>
#include <memory>
#include <string>

#include "Module.hpp"
#include "ModuleManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

using namespace allpix;

std::mutex Event::stats_mutex_;

Event::Event(Messenger& messenger, uint64_t event_num, uint64_t seed) : number(event_num), seed_(seed) {
    local_messenger_ = std::make_unique<LocalMessenger>(messenger);
}

void Event::set_and_seed_random_engine(RandomNumberGenerator* random_engine) {
    random_engine_ = random_engine;
    random_engine_->seed(seed_);
}

RandomNumberGenerator& Event::getRandomEngine() {
    if(random_engine_ == nullptr) {
        throw InvalidEventStateException("No PRNG available");
    }
    return *random_engine_;
}

void Event::store_random_engine_state() {
    if(random_engine_ != nullptr && state_.rdbuf()->in_avail() == 0) {
        LOG(PRNG) << "Storing PRNG state in event";
        state_ << *random_engine_;
    }
}

void Event::restore_random_engine_state() {
    if(random_engine_ != nullptr && state_.rdbuf()->in_avail() != 0) {
        LOG(PRNG) << "Restoring PRNG state from event";
        state_ >> *random_engine_;
        state_.clear();
    }
}

LocalMessenger* Event::get_local_messenger() const { return local_messenger_.get(); }
