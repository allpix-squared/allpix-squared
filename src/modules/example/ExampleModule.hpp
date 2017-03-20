/*
 * Example module with name 'example'
 *
 * NOTE: the factory builder is not included here
 *
 * Possible instantiation in the configuration file would be:
 * [example]
 * param = "test"
 */

#include <core/messenger/Messenger.hpp>
#include <core/module/Module.hpp>
#include <core/utils/log.h>

#include <memory>
#include <string>

namespace allpix {
    // definitions of messages
    // WARNING: definition of the messages should never be part of a module
    class InputMessage : public Message {
    public:
        // NOTE: in a real message the output is of course not fixed
        std::string get() { return "an input message"; }
    };
    class OutputMessage : public Message {
    public:
        explicit OutputMessage(std::string msg) : msg_(std::move(msg)) {}

        std::string get() { return msg_; }

    private:
        std::string msg_;
    };

    // define the module inheriting from the module base class
    // WARNING: the modules should implement their methods in the source file instead
    class ExampleModule : public Module {
    public:
        // provide a static const variable of type string (required!)
        static const std::string name;

        // constructor should take a pointer to AllPix and a Configuration as input
        ExampleModule(AllPix* apx, Configuration config) : Module(apx), message_(nullptr) {
            // print a configuration parameter of type string to the logger
            LOG(DEBUG) << "my string parameter is " << config.get<std::string>("param", "<undefined>");

            // bind a variable to a specific message type that is automatically assigned if it is dispatched
            getMessenger()->bindSingle(this, &ExampleModule::message_);
        }

        // method that will be run where the module should do its computations and possibly dispatch their results as a
        // message
        void run() override {
            // check if received a message
            if(message_) {
                // print the message
                LOG(DEBUG) << "received a message: " << message_.get();
            } else {
                LOG(DEBUG) << "did not receive any message before run...";
            }

            // construct my own message
            OutputMessage msg("my output message");

            // dispatch my message
            getMessenger()->dispatchMessage(msg);
        }

    private:
        std::shared_ptr<InputMessage> message_;
    };

    // set the name of the module
    // WARNING: this module name should not be set in the header
    const std::string ExampleModule::name = "example"; // NOLINT
}
