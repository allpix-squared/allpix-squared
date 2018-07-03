#ifndef ALLPIX_MODULE_MESSAGESTORAGE_H
#define ALLPIX_MODULE_MESSAGESTORAGE_H

#include "../messenger/Messenger.hpp"

namespace allpix {

    class MessageStorage {
        public:
            /**
             * @brief Constructor
             */
            explicit MessageStorage(Messenger::DelegateMap delegates)
                : delegates_(delegates) {}

            DelegateVariants& fetch_for(Module* module);

            bool is_satisfied(Module* module) const;

            /**
             * @brief Dispatches a message
             * @param source Module dispatching the message
             * @param message Pointer to the message to dispatch
             * @param name Optional message name (defaults to - indicating that it should dispatch to the module output
             * parameter)
             */
            /* template <typename T> */
            void dispatchMessage(Module* source, std::shared_ptr<BaseMessage> message, const std::string& name = "-") {
                dispatch_message(source, message, name);
            }

            template <typename Msg>
            std::shared_ptr<Msg> fetchMessage(Module* module) {
                return std::dynamic_pointer_cast<Msg>(mpark::get<std::shared_ptr<BaseMessage>>(fetch_for(module)));
            }

            template <typename Msg>
            std::vector<std::shared_ptr<Msg>> fetchMultiMessage(Module* module) {
                auto base_messages = mpark::get<std::vector<std::shared_ptr<BaseMessage>>>(fetch_for(module));
                std::vector<std::shared_ptr<Msg>> derived_messages;
                for (auto& message : base_messages) {
                    derived_messages.push_back(std::dynamic_pointer_cast<Msg>(message));
                }

                return derived_messages;
            }

        private:
            void dispatch_message(Module* source, std::shared_ptr<BaseMessage> message, std::string name);
            bool dispatch_message(Module* source, const std::shared_ptr<BaseMessage>& message, const std::string& name, const std::string& id);

            // What are all modules listening to?
            Messenger::DelegateMap delegates_;

            std::map<std::string, DelegateVariants> messages_;
            std::map<std::string, bool> satisfied_modules_;

            std::vector<std::shared_ptr<BaseMessage>> sent_messages_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_MESSAGESTORAGE_H */
