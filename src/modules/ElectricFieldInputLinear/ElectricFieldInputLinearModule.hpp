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

namespace allpix {
    // define the module inheriting from the module base class
    class ElectricFieldInputLinearModule : public Module {
    public:
        // constructor of the module
        ElectricFieldInputLinearModule(Configuration config, Messenger*, std::shared_ptr<Detector>);

        // read the electric field and set it for the detectors
        void init() override;

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
