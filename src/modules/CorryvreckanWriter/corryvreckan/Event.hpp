/**
 * @file
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRYVRECKAN_EVENT_H
#define CORRYVRECKAN_EVENT_H 1

#include "Object.hpp"

namespace corryvreckan {

    class Event : public Object {

    public:
        enum class Position {
            UNKNOWN, // StandardEvent position unknown
            BEFORE,  // StandardEvent is before current event
            DURING,  // StandardEvent is during current event
            AFTER,   // StandardEvent is after current event
        };

        /**
         * @brief Default constructor
         */
        Event() = default;

        /**
         * @brief Constructor for event bojects
         * @param start        Start timestamp of the event
         * @param end          End timestamp of the event
         * @param trigger_list Optional list of triggers assigned to this event, containing their ID and timestamps
         */
        Event(double start, double end, std::map<uint32_t, double> trigger_list = std::map<uint32_t, double>())
            : Object(start), end_(end), trigger_list_(std::move(trigger_list)) {};

        /**
         * @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(Event); }

        /**
         * @brief Get the timestamp of the start of the event
         * @return Start timestamp of the event
         */
        double start() const;

        /**
         * @brief Get the timestamp of the end of the event
         * @return End timestamp of the event
         */
        double end() const;

        /**
         * @brief Get the duration of the event
         * @return Duration of the event
         */
        double duration() const;

        /**
         * @brief Add a new trigger ID to this event
         * @param trigger_id ID of the trigger to be added
         * @param trigger_ts Timestamp corresponding to the trigger
         *
         * Trigger IDs are only added if they do not exist yet. Adding the same trigger ID twice will not change the
         * corresponding timestamp, the list remains unchanged.
         **/
        void addTrigger(uint32_t trigger_id, double trigger_ts);

        /**
         * @brief Check if trigger ID exists in current event
         * @param trigger_id ID of the trigger to be checked for
         * @return Bool whether trigger ID was found
         **/
        bool hasTriggerID(uint32_t trigger_id) const;

        /**
         * @brief Get trigger timestamp corresponding to a given trigger ID
         * @param trigger_id ID of the trigger for which the timestamp shall be returned
         * @return Timestamp corresponding to the trigger
         **/
        double getTriggerTime(uint32_t trigger_id) const;

        /**
         * @brief Returns position of a timestamp relative to the current event
         *
         * This function allows to assess whether a timestamp lies before, during or after the defined event.
         * @param  frame_start Timestamp to get position for
         * @return             Position of the given timestamp with respect to the defined event.
         */
        Position getTimestampPosition(double timestamp) const;

        /**
         * @brief Returns position of a time frame defined by a start and end point relative to the current event
         *
         * This function allows to assess whether a time frame lies before, during or after the defined event. There are two
         * options of interpretation. The inclusive interpretation will return "during" as soon as there is some overlap
         * between the frame and the event, i.e. as soon as the end of the frame is later than the event start or as soon as
         * the frame start is before the event end. In the exclusive mode, the frame will be classified as "during" only if
         * start and end are both within the defined event. The function returns UNKNOWN if the end of the given time frame
         * is before its start.
         * @param  frame_start Start timestamp of the frame
         * @param  frame_end   End timestamp of the frame
         * @param  inclusive   Boolean to select inclusive or exclusive mode
         * @return             Position of the given time frame with respect to the defined event.
         */
        Position getFramePosition(double frame_start, double frame_end, bool inclusive = true) const;

        /**
         * @brief Returns position of a given trigger ID with respect to the currently defined event.
         *
         * If the given trigger ID is smaller than the smallest trigger ID known to the event, BEFORE is returned. If the
         * trigger ID is larger than the largest know ID, AFTER is returned. If the trigger ID is known to the event, DURING
         * is returned. UNKNOWN is returned if either no trigger ID is known to the event or if the given ID lies between the
         * smallest and larges known ID but is not part of the event.
         * @param  trigger_id Given trigger ID to check
         * @return            Position of the given trigger ID with respect to the defined event.
         */
        Position getTriggerPosition(uint32_t trigger_id) const;

        /**
         * @brief Retrieve list with all triggers know to the event
         * @return Map of tirgger IDs with their corresponding timestamps
         */
        std::map<uint32_t, double> triggerList() const;

        /**
         * @brief Print an ASCII representation of Pixel to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

    protected:
        // Timestamp of the end of the event
        double end_;

        // List with all triggers known to the event, containing the trigger ID and its timestamp
        std::map<uint32_t, double> trigger_list_{};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Event, 6); // NOLINT
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_EVENT_SLICE_H
