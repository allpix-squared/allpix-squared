#include "core/module/Event.hpp"

namespace allpix {
    template <typename T> void BufferedModule<T>::run(Event* event) {
        // Fetch the user specified data from the event
        auto data = fetch_event_data(event);

        // Get the expected event number
        auto event_number = event->number;
        auto next_event = next_event_to_write_ + 1;

        // Check if the new event is in the expected order or not
        if(next_event_to_write_.compare_exchange_strong(event_number, next_event)) {
            LOG(STATUS) << "Writing Event " << event_number;
            // Process the in order event
            run_inorder(event_number, data);

            // Flush any in order buffered events
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            flush_buffered_events();
        } else {
            LOG(STATUS) << "Buffering event " << event->number;
            // Buffer out of order events to write them later
            std::lock_guard<std::mutex> lock(buffer_mutex_);
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

        while((iter = buffered_events_.find(next_event_to_write_)) != buffered_events_.end()) {
            // Process the buffered event
            run_inorder(iter->first, iter->second);

            // Remove it from buffer
            buffered_events_.erase(iter);

            // Move to the next event
            next_event_to_write_++;
        }
    }
}