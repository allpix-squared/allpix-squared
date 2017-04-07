/**
 * @author Paul Schuetze <paul.schuetze@desy.de>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <memory>
#include <random>
#include <string>

#include <Math/Vector3D.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/ChargeDeposit.hpp"

namespace allpix {
    // define the module to inherit from the module base class
    class SimplePropagationModule : public Module {
    public:
        // name of the module
        static const std::string name;

        // constructor and destructor
        SimplePropagationModule(Configuration, Messenger*, std::shared_ptr<Detector>);
        ~SimplePropagationModule() override;

        // do the propagation of the charge deposits
        void run() override;

    private:
        // propagate a single charge
        ROOT::Math::XYZPoint propagate(const ROOT::Math::XYZPoint& pos);

        // random generator for this module
        std::mt19937_64 random_generator_;

        // configuration for this module
        Configuration config_;

        // attached detector and pixel detector model
        std::shared_ptr<Detector> detector_;
        std::shared_ptr<PixelDetectorModel> model_;

        // deposits for a specific detector
        std::shared_ptr<ChargeDepositMessage> deposits_message_;
    };

} // namespace allpix
