#ifndef ALLPIX_MODULE_MESSAGESTORAGE_H
#define ALLPIX_MODULE_MESSAGESTORAGE_H

#include <map>
#include <memory>
#include <string>
#include <typeinfo>

#include "../messenger/delegates.h"
#include "Module.hpp"

namespace allpix {
    class Module;
    class Event;

    using DelegateMap = std::map<std::type_index, std::map<std::string, std::list<std::shared_ptr<BaseDelegate>>>>;

    class MessageStorage {
        friend class Event;

    private:
        /**
         * @brief Constructor
         */
        explicit MessageStorage(DelegateMap& delegates);

        /**
         * @brief Prepare storage for a module
         */
        MessageStorage& using_module(Module* module);

        /**
         * @brief Check if a module is satisfied for running (all required messages received)
         * @return True if satisfied, false otherwise
         */
        bool is_satisfied(Module* module) const;

        void dispatch_message(Module* source, std::shared_ptr<BaseMessage> message, std::string name);
        bool dispatch_message(Module* source,
                              const std::shared_ptr<BaseMessage>& message,
                              const std::string& name,
                              const std::string& id);

        // What are all modules listening to?
        DelegateMap& delegates_;

        std::map<std::string, DelegateTypes> messages_;

        // Currently active module
        Module* module_;

        std::map<std::string, bool> satisfied_modules_;

        std::vector<std::shared_ptr<BaseMessage>> sent_messages_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_MESSAGESTORAGE_H */
