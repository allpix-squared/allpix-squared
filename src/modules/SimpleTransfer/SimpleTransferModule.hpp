/*
 * Module to read electric fields in the INIT format
 * See https://github.com/simonspa/pixelav
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/PropagatedCharge.hpp"

namespace allpix {
    // define the module inheriting from the module base class
    class SimpleTransferModule : public Module {
    public:
        // name of the module
        static const std::string name;

        // constructor of the module
        SimpleTransferModule(Configuration config, Messenger*, std::shared_ptr<Detector>);

        // read the electric field and set it for the detectors
        // FIXME: this should not run again for every event
        void run() override;

    private:
        // configuration for this reader
        Configuration config_;

        // pointer to the messenger
        Messenger* messenger_;

        // linked detector for this instantiation
        std::shared_ptr<Detector> detector_;

        // propagated deposits for a specific detector
        std::shared_ptr<PropagatedChargeMessage> propagated_message_;
    };
} // namespace allpix
