/*
 * Example module with name 'example'
 *
 * NOTE: the factory builder is not included here
 *
 * Possible instantiation in the configuration file would be:
 * [example]
 * param = "test"
 */

// stl includes
#include <memory>
#include <string>
#include <utility>

// include the dependent objects
#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/BaseMessage.hpp"
#include "core/messenger/Messenger.hpp"

// include the module base class
#include <core/module/Module.hpp>

#include <memory>
#include <string>

namespace allpix {
    // definitions of messages
    // WARNING: definition of the messages should never be part of a module in real modules (and template Message should be
    // preferred)
    class InputMessage : public BaseMessage {
    public:
        // NOTE: in a real message the output is of course not fixed
        std::string get() { return "an input message"; }
    };
    class OutputMessage : public BaseMessage {
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

        // constructor should take a pointer to the Configuration, the Messenger and the Geometry Manager
        ExampleModule(Configuration config, Messenger*, GeometryManager*);

        // method that should do the necessary computations and possibly dispatch their results as a message
        void run() override;

    private:
        // messenger to emit test message
        Messenger* messenger_;

        // message to receive
        std::shared_ptr<InputMessage> message_;
    };
} // namespace allpix
