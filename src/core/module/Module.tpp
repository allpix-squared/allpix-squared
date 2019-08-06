#include "core/module/Event.hpp"

namespace allpix {
    template <typename T> void BufferedModule<T>::run(Event* event) {
        // Fetch the user specified data from the event
        auto data = fetch_event_data(event);

        // Get the expected event number
        auto event_number = event->number;

        // Check if the new event is in the expected order or not
        std::lock_guard<std::mutex> lock(buffer_mutex_);
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