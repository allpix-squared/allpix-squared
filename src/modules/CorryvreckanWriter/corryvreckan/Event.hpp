#ifndef CORRYVRECKAN_EVENT_H
#define CORRYVRECKAN_EVENT_H 1

#include "Object.hpp"

namespace corryvreckan {

    class Event : public Object {

    public:
        // Constructors and destructors
        Event() = default;
        Event(double start, double end, std::map<uint32_t, double> trigger_list = std::map<uint32_t, double>())
            : Object(start), end_(end), trigger_list_(std::move(trigger_list)){};

        double start() const { return timestamp(); };
        double end() const { return end_; };
        double duration() const { return (end_ - timestamp()); };

        /**
         * @brief Add a new trigger ID to this event
         * @param trigger_id ID of the trigger to be added
         * @param trigger_ts Timestamp corresponding to the trigger
         *
         * Trigger IDs are only added if they do not exist yet. Adding the same trigger ID twice will not change the
         *corresponding timestamp, the list remains unchanged.
         **/
        void addTrigger(uint32_t trigger_id, double trigger_ts) { trigger_list_.emplace(trigger_id, trigger_ts); }

        /**
         * @brief Check if trigger ID exists in current event
         * @param trigger_id ID of the trigger to be checked for
         * @return Bool whether trigger ID was found
         **/
        bool hasTriggerID(uint32_t trigger_id) const { return (trigger_list_.find(trigger_id) != trigger_list_.end()); }

        /**
         * @brief Get trigger timestamp corresponding to a given trigger ID
         * @param trigger_id ID of the trigger for which the timestamp shall be returned
         * @return Timestamp corresponding to the trigger
         **/
        double getTriggerTime(uint32_t trigger_id) const { return trigger_list_.find(trigger_id)->second; }

        std::map<uint32_t, double> triggerList() const { return trigger_list_; }

        /**
         * @brief Print an ASCII representation of Pixel to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

    protected:
        double end_;
        std::map<uint32_t, double> trigger_list_{};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Event, 4)
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_EVENT_SLICE_H
