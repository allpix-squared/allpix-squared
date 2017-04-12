// stl include
#include <string>

// module include
#include "ExampleModule.hpp"

// framework includes
#include <core/messenger/Messenger.hpp>
#include <core/utils/log.h>

using namespace allpix;

// set the name of the module
const std::string ExampleModule::name = "Example";

// constructor to load the module
ExampleModule::ExampleModule(Configuration config, Messenger* messenger, GeometryManager*)
    : messenger_(messenger), message_(nullptr) {
    // print a configuration parameter of type string to the logger
    LOG(DEBUG) << "my string parameter 'param' is equal to " << config.get<std::string>("param", "<undefined>");

    // bind a variable to a specific message type that is automatically assigned if it is dispatched
    messenger->bindSingle(this, &ExampleModule::message_);
}

// run method fetching a message and outputting its own
void ExampleModule::run() {
    // check if received a message
    if(message_) {
        // print the message
        LOG(DEBUG) << "received a message: " << message_.get();
    } else {
        // no message received
        LOG(DEBUG) << "did not receive any message before run...";
    }

    // construct my own message
    OutputMessage msg("my output message");

    // dispatch my message
    messenger_->dispatchMessage(msg);
}
