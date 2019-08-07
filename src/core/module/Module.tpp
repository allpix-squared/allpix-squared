#include "core/module/Event.hpp"

namespace allpix {
    template <typename T> void BufferedModule<T>::run(Event* event) {
        // Fetch the user specified data from the event
        auto data = fetch_event_data(event);

        // Get the expected event number
        auto event_number = event->number;

        std::lock_guard<std::mutex> lock(buffer_mutex_);

        // Make sure we are waiting for the right event
        next_event_to_write_ = get_next_event(next_event_to_write_);

        // Check if the new event is in the expected order or not
        if(next_event_to_write_ == event_number) {
            LOG(TRACE) << "Writing Event " << event_number;

            // Process the in order event
            run_inorder(event_number, data);

            // Advance the expected event
            next_event_to_write_++;

            // Flush any in order buffered events
            flush_buffered_events();
        } else {
            LOG(TRACE) << "Buffering event " << event->number;

            // Buffer out of order events to write them later
            buffered_events_.insert(std::make_pair(event->number, std::move(data)));
        }
    }

    template <typename T> void BufferedModule<T>::finalize() {
        // Write any remaining buffered events
        flush_buffered_events();

        // Make sure everything was written
        if(!buffered_events_.empty()) {
            throw ModuleError("Buffered Events where not written to output");
        }

        // Call use finalize
        finalize_module();
    }

    template <typename T> void BufferedModule<T>::flush_buffered_events() {
        typename std::map<unsigned int, T>::iterator iter;

        next_event_to_write_ = get_next_event(next_event_to_write_);
        while((iter = buffered_events_.find(next_event_to_write_)) != buffered_events_.end()) {
            LOG(TRACE) << "Writing bufffered event " << iter->first;

            // Process the buffered event
            run_inorder(iter->first, iter->second);

            // Remove it from buffer
            buffered_events_.erase(iter);

            // Move to the next event
            next_event_to_write_ = get_next_event(next_event_to_write_ + 1);
        }
    }

    template <typename T> unsigned int BufferedModule<T>::get_next_event(unsigned int current) {
        std::set<unsigned int>::iterator iter;
        auto next = current;

        // Check sequentially if events were skipped
        while((iter = skipped_events_.find(next)) != skipped_events_.end()) {
            LOG(TRACE) << "Event skipped " << *iter;

            // Remove from the skipped set because no point of checking it again
            skipped_events_.erase(iter);

            // Move on...
            next++;
        }
        return next;
    }

    template <typename T> void BufferedModule<T>::skip_event(unsigned int event) {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        skipped_events_.insert(event);
    }
}