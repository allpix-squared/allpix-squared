#ifndef ALLPIX_MODULE_MESSAGESTORAGE_H
#define ALLPIX_MODULE_MESSAGESTORAGE_H

#include "../messenger/Messenger.hpp"

namespace allpix {
    class Module;

    using DelegateMap = std::map<std::type_index, std::map<std::string, std::list<std::shared_ptr<BaseDelegate>>>>;

    class MessageStorage {
        public:
            /**
             * @brief Constructor
             */
            explicit MessageStorage(DelegateMap delegates)
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
            template <typename T>
            void dispatchMessage(Module* source, std::shared_ptr<T> message, const std::string& name = "-") {
                static_assert(std::is_base_of<BaseMessage, T>::value, "Dispatched message should inherit from Message class");
                dispatch_message(source, message, name);
            }

            template <typename T>
            std::shared_ptr<T> fetchMessage() {
                static_assert(std::is_base_of<BaseMessage, T>::value, "Fetched message should inherit from Message class");
                return std::dynamic_pointer_cast<T>(mpark::get<std::shared_ptr<BaseMessage>>(message_));
            }

            template <typename T>
            std::vector<std::shared_ptr<T>> fetchMultiMessage() {
                static_assert(std::is_base_of<BaseMessage, T>::value, "Fetched message should inherit from Message class");
                auto base_messages = mpark::get<std::vector<std::shared_ptr<BaseMessage>>>(message_);
                std::vector<std::shared_ptr<T>> derived_messages;
                for (auto& message : base_messages) {
                    derived_messages.push_back(std::dynamic_pointer_cast<T>(message));
                }

                return derived_messages;
            }

            // ... nah..
            MessageStorage& using_module(Module* module) {
                message_ = fetch_for(module);
                return *this;
            }

        private:
            void dispatch_message(Module* source, std::shared_ptr<BaseMessage> message, std::string name);
            bool dispatch_message(Module* source, const std::shared_ptr<BaseMessage>& message, const std::string& name, const std::string& id);

            // What are all modules listening to?
            DelegateMap delegates_;

            std::map<std::string, DelegateVariants> messages_;
            DelegateVariants message_;
            std::map<std::string, bool> satisfied_modules_;

            std::vector<std::shared_ptr<BaseMessage>> sent_messages_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_MESSAGESTORAGE_H */
