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

#include "objects/PixelCharge.hpp"
#include "objects/PropagatedCharge.hpp"

namespace allpix {
    // define the module inheriting from the module base class
    class SimpleTransferModule : public Module {
    public:
        // constructor of the module
        SimpleTransferModule(Configuration config, Messenger*, std::shared_ptr<Detector>);

        // read the electric field and set it for the detectors
        void run(unsigned int) override;

        // print statistics
        void finalize() override;

    private:
        // compare two pixels for the pixel map
        struct pixel_cmp {
            bool operator()(const PixelCharge::Pixel& p1, const PixelCharge::Pixel& p2) const {
                if(p1.x() == p2.x()) {
                    return p1.y() < p2.y();
                }
                return p1.x() < p2.x();
            }
        };

        // configuration for this reader
        Configuration config_;

        // pointer to the messenger
        Messenger* messenger_;

        // linked detector for this instantiation
        std::shared_ptr<Detector> detector_;

        // propagated deposits for a specific detector
        std::shared_ptr<PropagatedChargeMessage> propagated_message_;

        // statistics
        unsigned int total_transferrred_charges_{};
        std::set<PixelCharge::Pixel, pixel_cmp> unique_pixels_;
    };
} // namespace allpix
