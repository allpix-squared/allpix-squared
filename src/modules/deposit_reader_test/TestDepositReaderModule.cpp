/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <iomanip>

#include "TestDepositReaderModule.hpp"

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

#include "core/utils/unit.h"
#include "tools/ROOT.h"

using namespace allpix;

const std::string TestDepositReaderModule::name = "deposit_reader_test";

TestDepositReaderModule::TestDepositReaderModule(Configuration config, Messenger* messenger, GeometryManager*)
    : config_(std::move(config)), deposit_messages_() {
    // fetch deposits
    messenger->bindMulti(this, &TestDepositReaderModule::deposit_messages_);
}
TestDepositReaderModule::~TestDepositReaderModule() = default;

// print the deposits
void TestDepositReaderModule::run() {
    LOG(INFO) << "READ DEPOSITS";

    LOG(INFO) << "got " << deposit_messages_.size() << " deposits";
    for(auto& message : deposit_messages_) {
        LOG(DEBUG) << " list of deposits";
        for(auto& deposit : message->getData()) {
            auto pos = deposit.getPosition();

            auto x = Units::convert(pos.x(), "um");
            auto y = Units::convert(pos.y(), "um");
            auto z = Units::convert(pos.z(), "um");

            LOG(DEBUG) << std::fixed << std::setprecision(5) << deposit.getCharge()
                       << " units of charge deposited at point (" << x << "um," << y << "um," << z << "um) in detector "
                       << message->getDetector()->getName();
        }
    }
}
