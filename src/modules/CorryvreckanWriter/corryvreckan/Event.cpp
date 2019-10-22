#include "Event.hpp"

using namespace corryvreckan;

void Event::print(std::ostream& out) const {
    out << "Start: " << start() << std::endl;
    out << "End:   " << end();
    if(!trigger_list_.empty()) {
        out << std::endl << "Trigger list: ";
        for(auto& trg : trigger_list_) {
            out << std::endl << trg.first << ": " << trg.second;
        }
    }
}
