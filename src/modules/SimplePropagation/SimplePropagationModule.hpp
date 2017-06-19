/**
 * @author Paul Schuetze <paul.schuetze@desy.de>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <memory>
#include <random>
#include <string>
#include <vector>

#include <Math/Point3D.h>
#include <TFile.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

namespace allpix {
    // define the module to inherit from the module base class
    class SimplePropagationModule : public Module {
    public:
        // constructor and destructor
        SimplePropagationModule(Configuration, Messenger*, std::shared_ptr<Detector>);

        // do the propagation of the charge deposits
        void run(unsigned int event_num) override;

        // finalize debug plots
        void finalize() override;

    private:
        // create debug plots
        void create_output_plots(unsigned int event_num);

        // propagate a single charge
        std::pair<ROOT::Math::XYZPoint, double> propagate(const ROOT::Math::XYZPoint& pos);

        // random generator for this module
        std::mt19937_64 random_generator_;

        // configuration for this module
        Configuration config_;
        // local copies of configuration parameters to avoid unnecessary lookup:
        double temperature, timestep_min, timestep_max, timestep_start, target_spatial_precision, output_plots_step;
        bool output_plots;

        // pointer to the messenger
        Messenger* messenger_;

        // attached detector and detector model
        std::shared_ptr<Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        // deposits for a specific detector
        std::shared_ptr<DepositedChargeMessage> deposits_message_;

        // statistics
        unsigned int total_propagated_charges_{};
        unsigned int total_steps_{};
        long double total_time_{};

        // Precalculated values for electron mobility
        double electron_Vm_;
        double electron_Ec_;
        double electron_Beta_;

        // debug list of points to plot
        std::vector<std::pair<PropagatedCharge, std::vector<ROOT::Math::XYZPoint>>> output_plot_points_;
    };

} // namespace allpix
