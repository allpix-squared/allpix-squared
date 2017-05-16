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

TestDepositReaderModule::TestDepositReaderModule(Configuration config, Messenger* messenger, GeometryManager*)
    : config_(std::move(config)) {
    // fetch deposits
    messenger->bindMulti(this, &TestDepositReaderModule::deposit_messages_);
}

// print the deposits
void TestDepositReaderModule::run(unsigned int) {
    LOG(INFO) << "Got deposits in " << deposit_messages_.size() << " detectors";
    for(auto& message : deposit_messages_) {
        LOG(INFO) << "set of " << message->getData().size() << " deposits in detector " << message->getDetector()->getName();
        for(auto& deposit : message->getData()) {
            auto pos = deposit.getPosition();

            auto x = Units::convert(pos.x(), "um");
            auto y = Units::convert(pos.y(), "um");
            auto z = Units::convert(pos.z(), "um");

            LOG(INFO) << " " << std::fixed << std::setprecision(5) << deposit.getCharge()
                      << " charges deposited at position (" << x << "um," << y << "um," << z << "um)";
        }
    }
    deposit_messages_.clear();
}
