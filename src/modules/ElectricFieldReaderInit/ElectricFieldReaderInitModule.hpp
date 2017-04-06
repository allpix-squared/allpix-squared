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

// use the allpix namespace
namespace allpix {
    // define the module inheriting from the module base class
    class ElectricFieldReaderInitModule : public Module {
    public:
        // name of the module
        static const std::string name;

        // constructor of the module
        ElectricFieldReaderInitModule(Configuration config, Messenger*, std::shared_ptr<Detector>);

        // read the electric field and set it for the detectors
        // FIXME: this should not run again for every event
        void run() override;

    private:
        using FieldData = std::pair<std::shared_ptr<std::vector<double>>, std::array<size_t, 3>>;

        // get the electric field from a file name
        static FieldData get_by_file_name(const std::string& name, Detector&);
        static std::map<std::string, FieldData> field_map_;

        // configuration for this reader
        Configuration config_;

        // linked detector for this instantiation
        std::shared_ptr<Detector> detector_;
    };
} // namespace allpix
