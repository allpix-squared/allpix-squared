/**
 * @file
 * @brief Implementation of event object
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <cstdint>
#include <map>
#include <ostream>

#include "Event.hpp"

using namespace corryvreckan;

double Event::start() const { return timestamp(); }

double Event::end() const { return end_; }

double Event::duration() const { return (end_ - timestamp()); }

void Event::addTrigger(uint32_t trigger_id, double trigger_ts) { trigger_list_.emplace(trigger_id, trigger_ts); }

bool Event::hasTriggerID(uint32_t trigger_id) const { return (trigger_list_.find(trigger_id) != trigger_list_.end()); }

double Event::getTriggerTime(uint32_t trigger_id) const { return trigger_list_.find(trigger_id)->second; }

std::map<uint32_t, double> Event::triggerList() const { return trigger_list_; }

Event::Position Event::getTimestampPosition(double timestamp) const {
    if(timestamp < start()) {
        return Position::BEFORE;
    }
    if(end() < timestamp) {
        return Position::AFTER;
    }
    return Position::DURING;
}

Event::Position Event::getFramePosition(double frame_start, double frame_end, bool inclusive) const {
    // The frame is ill-defined, we have no idea what to do with this data:
    if(frame_end < frame_start) {
        return Position::UNKNOWN;
    }

    if(inclusive) {
        // Return DURING if there is any overlap
        if(frame_end < start()) {
            return Position::BEFORE;
        }
        if(end() < frame_start) {
            return Position::AFTER;
        }
        return Position::DURING;
    } else {
        // Return DURING only if fully covered
        if(frame_start < start()) {
            return Position::BEFORE;
        }
        if(end() < frame_end) {
            return Position::AFTER;
        }
        return Position::DURING;
    }
}

Event::Position Event::getTriggerPosition(uint32_t trigger_id) const {
    // If we have no triggers we cannot make a statement:
    if(trigger_list_.empty()) {
        return Position::UNKNOWN;
    }

    if(hasTriggerID(trigger_id)) {
        return Position::DURING;
    }

    if(trigger_list_.upper_bound(trigger_id) == trigger_list_.begin()) {
        // Upper bound returns first element that is greater than the given key - in this case, the first map element
        // is greater than the provided trigger number - which consequently is before the event.
        return Position::BEFORE;
    }

    if(trigger_list_.lower_bound(trigger_id) == trigger_list_.end()) {
        // Lower bound returns the first element that is *not less* than the given key - in this case, even the last
        // map element is less than the trigger number - which consequently is after the event.
        return Position::AFTER;
    }

    // We have not enough information to provide position information.
    return Position::UNKNOWN;
}

void Event::print(std::ostream& out) const {
    out << "Start: " << start() << '\n';
    out << "End:   " << end();
    if(!trigger_list_.empty()) {
        out << '\n' << "Trigger list: ";
        for(const auto& trg : trigger_list_) {
            out << '\n' << trg.first << ": " << trg.second;
        }
    }
}
