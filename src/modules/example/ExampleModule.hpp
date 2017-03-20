/*
 * Example module with name 'example'
 *
 * NOTE: the factory builder is not included here
 *
 * Possible instantiation in the configuration file would be:
 * [example]
 * param = "test"
 */

// include the module base class and the configuration object
#include <core/config/Configuration.hpp>
#include <core/module/Module.hpp>

// include the message base class
#include <core/messenger/Message.hpp>

#include <memory>
#include <string>

namespace allpix {
    // definitions of messages
    // WARNING: definition of the messages should never be part of a module in real modules
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
    class ExampleModule : public Module {
    public:
        // provide a static const variable of type string (required!)
        static const std::string name;

        // constructor should take a pointer to AllPix and a Configuration as input
        ExampleModule(AllPix* apx, Configuration config);

        // method that should do the necessary computations and possibly dispatch their results as a message
        void run() override;

    private:
        std::shared_ptr<InputMessage> message_;
    };
}
