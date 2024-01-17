/**
 * @file
 * @brief Implementation of RCE Writer Module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "RCEWriterModule.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <TBranchElement.h>
#include <TClass.h>
#include <TDecompSVD.h>
#include <TDirectory.h>
#include <TMatrixD.h>

#include "core/utils/log.h"
#include "core/utils/type.h"
#include "core/utils/unit.h"
#include "objects/Object.hpp"
#include "objects/objects.h"

using namespace allpix;

static double compute_model_relative_radlength(const DetectorModel& model) {
    const double X0_SI = Units::get(9.370, "cm");
    const double X0_IN = Units::get(1.211, "cm");

    double total = 0;
    // helper functions to add component to total length w/ logging
    auto add = [&](const char* what, double X0, double x) {
        double xX0 = x / X0;
        LOG(DEBUG) << "  " << what << " x/X0 = " << Units::display(x, {"um", "mm", "cm"}) << "/"
                   << Units::display(X0, {"um", "mm", "cm", "m"}) << " = " << xX0;
        total += xX0;
    };

    LOG(DEBUG) << "model '" << model.getType() << "' radiation length:";

    // compute contributions from sensor and chip
    add("sensor", X0_SI, model.getSensorSize().z());
    add("chip", X0_SI, model.getChipSize().z());

    // compute contributions from bumps if available
    const auto* hybrid = dynamic_cast<const HybridAssembly*>(model.getAssembly().get());
    if(hybrid != nullptr) {
        // average the bump material over the full pixel size.
        auto bump_radius = std::max(hybrid->getBumpSphereRadius(), hybrid->getBumpCylinderRadius());
        auto bump_height = hybrid->getBumpHeight();
        auto area_bump = M_PI * bump_radius * bump_radius;
        auto area_pixel = model.getPixelSize().x() * model.getPixelSize().y();
        // volume_bump = area_bump * thickness = area_pixel * effective_thickness
        auto relative_area = area_bump / area_pixel;
        LOG(DEBUG) << "  bump_height = " << Units::display(bump_height, "um") << " relative_area = " << relative_area;
        // TODO use typical solder combination instead of just indium
        add("bumps", X0_IN, relative_area * bump_height);
    }

    // TODO consider also support layers after material handling has been clarified

    LOG(DEBUG) << "  total x/X0 = " << total;
    return total;
}

// The Proteus sensor type is not identical w/ the Allpix² model since the
// latter only defines the geometry while the former also includes some
// digitization information. This leads to the situation where detectors
// with the same model can end up as different sensor types.

namespace {
    struct sensor_type {
        std::shared_ptr<const DetectorModel> model = nullptr;
        std::string name;
        std::string measurement = "pixel_binary";
        int value_max = (1 << 15); // ~32k, raw charge w/o digitization
    };
} // namespace

// Return the Proteus sensor type for each detector.
static std::vector<sensor_type>
list_sensor_types(const std::vector<std::string>& names, GeometryManager& geo_mgr, ConfigManager& cfg_mgr) {
    std::vector<sensor_type> sensor_types;

    for(const auto& name : names) {
        // Define a default sensor config
        sensor_type st;
        st.model = geo_mgr.getDetector(name)->getModel();
        st.name = st.model->getType();

        // Search for a corresponding digitizer configuration
        for(const auto& cfg : cfg_mgr.getInstanceConfigurations()) {
            const auto& module = cfg.getName();
            const auto& identifier = cfg.get<std::string>("identifier");

            // NOTE
            // Apart from the detector name, the identifier can contain
            // additional input/output components. In that case, the
            // configuration is a bit more complicated and can probably not
            // be translated to a simplified Proteus configuration anyways.
            // So, no need to check for that. (Thanks, Simon).

            // _linearX name because digitizer only supports linear ADC map
            if((module == "DefaultDigitizer") and (identifier == name)) {
                auto adc_resolution = cfg.get<int>("adc_resolution", 0);
                if(adc_resolution == 1) {
                    // binary pixels
                    st.name += "_linear1";
                    st.measurement = "pixel_binary";
                    st.value_max = 1;
                } else if(1 < adc_resolution) {
                    // pixel w/ digitized charge measurement
                    st.name += "_linear" + std::to_string(adc_resolution);
                    st.measurement = "pixel_tot";
                    st.value_max = (1 << adc_resolution) - 1;
                } else {
                    // no digitization, use defaults
                    st.name += "_raw";
                }
                // config can only appear once
                break;
            }
        }
        sensor_types.push_back(std::move(st));
    }
    return sensor_types;
}

static void print_device_sensor_type(std::ostream& os, const sensor_type& sensor_type) {
    const DetectorModel& model = *sensor_type.model;

    os << "[sensor_types." << sensor_type.name << "]\n";
    os << "cols = " << model.getNPixels().x() << "\n";
    os << "rows = " << model.getNPixels().y() << "\n";
    // TODO add timestamp_{min,max} once the timing is supported by digitizer
    os << "value_max = " << sensor_type.value_max << "\n";
    os << "pitch_col = " << model.getPixelSize().x() << "\n";
    os << "pitch_row = " << model.getPixelSize().y() << "\n";
    // TODO add pitch_timestamp once timing is supported by digitizer
    // thickness is the active thickness
    os << "thickness = " << model.getSensorSize().z() << "\n";
    // relative radiation length is for all material in the beam
    os << "x_x0 = " << compute_model_relative_radlength(model) << "\n";
    os << "measurement = \"" << sensor_type.measurement << "\"\n";
    os << "\n";
}

static void print_device_sensor(std::ostream& os, const std::string& name, const sensor_type& sensor_type) {
    os << "[[sensors]]\n";
    os << "name = \"" << name << "\"\n";
    os << "type = \"" << sensor_type.name << "\"\n";
    os << "\n";
}

static void
print_device(std::ostream& os, const std::vector<std::string>& names, GeometryManager& geo_mgr, ConfigManager& cfg_mgr) {
    // sensor type for each detector
    std::vector<sensor_type> sensor_types = list_sensor_types(names, geo_mgr, cfg_mgr);
    // reduce to unique sensor types
    std::vector<sensor_type> unique_types = sensor_types;
    std::sort(unique_types.begin(), unique_types.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
    unique_types.erase(
        std::unique(unique_types.begin(), unique_types.end(), [](const auto& a, const auto& b) { return a.name == b.name; }),
        unique_types.end());

    for(const auto& sensor_type : unique_types) {
        print_device_sensor_type(os, sensor_type);
    }
    for(size_t i = 0; i < std::min(names.size(), sensor_types.size()); ++i) {
        print_device_sensor(os, names[i], sensor_types[i]);
    }
}

/** Orthogonalize the coordinates definition using singular value decomposition.
 *
 * \param unit_u  Unit vector along the first local axis
 * \param unit_v  Unit vector along the second local axis
 *
 * The third local axis is derived from the first two by assuming a right-handed
 * coordinate system. The resulting rotation matrix is then orthogonalized
 * by finding the closest orthogonal matrix.
 */
static void orthogonalize(ROOT::Math::XYZVector& unit_u, ROOT::Math::XYZVector& unit_v) {
    // definition of a right-handed coordinate system
    auto unit_w = unit_u.Cross(unit_v);
    // resulting local-to-global rotation matrix
    TMatrixD rot(3, 3);
    rot(0, 0) = unit_u.x();
    rot(1, 0) = unit_u.y();
    rot(2, 0) = unit_u.z();
    rot(0, 1) = unit_v.x();
    rot(1, 1) = unit_v.y();
    rot(2, 1) = unit_v.z();
    rot(0, 2) = unit_w.x();
    rot(1, 2) = unit_w.y();
    rot(2, 2) = unit_w.z();
    // decompose
    TDecompSVD svd(rot);
    svd.Decompose();
    // nearest orthogonal matrix is defined by unit singular values
    rot = svd.GetU() * TMatrixD(svd.GetV()).T();

    unit_u.SetXYZ(rot(0, 0), rot(1, 0), rot(2, 0));
    unit_v.SetXYZ(rot(0, 1), rot(1, 1), rot(2, 1));
}

static void print_geometry_sensor(std::ostream& os, int index, const Detector& detector) {
    // Proteus uses the lower-left pixel edge closest to the geometric center
    // of the active pixel matrix as the reference position.
    // The position must be given in global coordinates
    auto size = detector.getModel()->getNPixels();
    auto pitch = detector.getModel()->getPixelSize();
    // pixel index in allpix is pixel center, i.e. pixel goes from (-0.5, 0.5)
    auto off_u = pitch.x() * (std::round(size.x() / 2.0) - 0.5);
    auto off_v = pitch.y() * (std::round(size.y() / 2.0) - 0.5);
    auto offset = detector.getGlobalPosition({off_u, off_v, 0});

    // Proteus defines the orientation of the sensor using two unit vectors
    // along the two local axes of the active matrix as seen in the global
    // system.
    // They are computed as difference vectors here to avoid a dependency
    // on the transformation implementation (which has led to errors before).
    auto zero = detector.getGlobalPosition({0, 0, 0});
    auto unit_u = (detector.getGlobalPosition({1, 0, 0}) - zero).Unit();
    auto unit_v = (detector.getGlobalPosition({0, 1, 0}) - zero).Unit();
    // try to fix round-off issues
    orthogonalize(unit_u, unit_v);

    os << "[[sensors]]\n";
    os << "id = " << index << '\n';
    os << "offset = [" << offset.x() << ", " << offset.y() << ", " << offset.z() << "]\n";
    os << "unit_u = [" << unit_u.x() << ", " << unit_u.y() << ", " << unit_u.z() << "]\n";
    os << "unit_v = [" << unit_v.x() << ", " << unit_v.y() << ", " << unit_v.z() << "]\n";
    os << '\n';
}

static void
print_geometry(std::ostream& os, const std::vector<std::string>& names, GeometryManager& geo_mgr, ConfigManager& cfg_mgr) {
    // extract (optional) beam information
    for(const auto& cfg : cfg_mgr.getModuleConfigurations()) {
        if(cfg.getName() == "DepositionGeant4") {
            auto energy = cfg.get<double>("source_energy", 0.0);
            auto dir = cfg.get<ROOT::Math::XYZVector>("beam_direction", {0, 0, 1});
            auto div = cfg.get<ROOT::Math::XYVector>("beam_divergence", {0, 0});

            os << "[beam]\n";
            os << "energy = " << energy / Units::get(1, "GeV") << "\n";
            os << "slope = [" << dir.x() / dir.z() << ", " << dir.y() / dir.z() << "]\n";
            os << "divergence = [" << div.x() << ", " << div.y() << "]\n";
            os << "\n";

            // deposition module is a unique module that appears at most once
            break;
        }
    }

    // per-detector geometry
    int identifier = 0;
    for(const auto& name : names) {
        print_geometry_sensor(os, identifier, *geo_mgr.getDetector(name));
        identifier += 1;
    }
}

static void write_proteus_config(const std::string& device_path,
                                 const std::string& geometry_path,
                                 const std::vector<std::string>& names,
                                 GeometryManager& geo_mgr,
                                 ConfigManager& cfg_mgr) {
    std::ofstream device_file(device_path);
    std::ofstream geometry_file(geometry_path);

    // ensure float values are not printed as integers
    device_file << std::showpoint;
    geometry_file << std::showpoint;
    // need full precision for geometry unit vector components
    geometry_file << std::setprecision(std::numeric_limits<double>::max_digits10);

    // device config
    // TODO use path relative to the device file
    device_file << "geometry = \"" << std::filesystem::canonical(geometry_path) << "\"\n";
    device_file << '\n';
    print_device(device_file, names, geo_mgr, cfg_mgr);

    // geometry config
    print_geometry(geometry_file, names, geo_mgr, cfg_mgr);
}

RCEWriterModule::RCEWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_mgr)
    : SequentialModule(config), messenger_(messenger), geo_mgr_(geo_mgr) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    assert(messenger && "messenger must be non-null");
    assert(geo_mgr && "geo_mgr must be non-null");

    // Bind to PixelHitMessage
    messenger_->bindMulti<PixelHitMessage>(this, MsgFlags::REQUIRED);

    config_.setDefault("file_name", "rce-data.root");
    // Use default names in Proteus
    config_.setDefault("device_file", "device.toml");
    config_.setDefault("geometry_file", "geometry.toml");
}

void RCEWriterModule::initialize() {
    // We need a sorted list of names to assign monotonic, numeric ids
    std::vector<std::string> detector_names;
    for(const auto& detector : geo_mgr_->getDetectors()) {
        detector_names.push_back(detector->getName());
    }
    std::sort(detector_names.begin(), detector_names.end());

    // Open output data file
    std::string path_data = createOutputFile(config_.get<std::string>("file_name"), "root");
    output_file_ = std::make_unique<TFile>(path_data.c_str(), "RECREATE");
    output_file_->cd();

    // Initialize the events tree
    event_tree_ = new TTree("Event", "");
    event_tree_->Branch("TimeStamp", &timestamp_);
    event_tree_->Branch("FrameNumber", &frame_number_);
    event_tree_->Branch("TriggerTime", &trigger_time_);
    event_tree_->Branch("TriggerOffset", &trigger_offset_);
    event_tree_->Branch("TriggerInfo", &trigger_info_);
    event_tree_->Branch("Invalid", &invalid_);

    // For each detector name, initialize an instance of SensorData
    int det_index = 0;
    for(const auto& detector_name : detector_names) {
        auto& sensor = sensors_[detector_name];

        LOG(TRACE) << "Sensor " << det_index << ", detector " << detector_name;

        // Create sensor directory
        std::string det_dir_name = "Plane" + std::to_string(det_index);
        TDirectory* detector = output_file_->mkdir(det_dir_name.c_str());
        detector->cd();

        // Initialize the tree and its branches
        sensor.tree = new TTree("Hits", "");
        sensor.tree->Branch("NHits", &sensor.nhits_);
        sensor.tree->Branch("PixX", &sensor.pix_x_, "PixX[NHits]/I");
        sensor.tree->Branch("PixY", &sensor.pix_y_, "PixY[NHits]/I");
        sensor.tree->Branch("Value", &sensor.value_, "Value[NHits]/I");
        sensor.tree->Branch("Timing", &sensor.timing_, "Timing[NHits]/I");
        sensor.tree->Branch("HitInCluster", &sensor.hit_in_cluster_, "HitInCluster[NHits]/I");

        det_index += 1;
    }

    // Write proteus config files
    auto device_path = createOutputFile(config_.get<std::string>("device_file"), "toml");
    auto geometry_path = createOutputFile(config_.get<std::string>("geometry_file"), "toml");
    write_proteus_config(device_path, geometry_path, detector_names, *geo_mgr_, *getConfigManager());
}

void RCEWriterModule::run(Event* event) {
    auto pixel_hit_messages = messenger_->fetchMultiMessage<PixelHitMessage>(this, event);

    // fill per-event data
    timestamp_ = 0;
    frame_number_ = event->number;
    trigger_time_ = 0;
    trigger_offset_ = 0;
    trigger_info_ = 0;
    invalid_ = false;
    event_tree_->Fill();
    LOG(TRACE) << "Wrote global event data";

    // reset all per-sensor trees
    for(auto& item : sensors_) {
        item.second.nhits_ = 0;
    }

    // Loop over the pixel hit messages
    for(const auto& hit_msg : pixel_hit_messages) {
        const auto& detector_name = hit_msg->getDetector()->getName();
        auto& sensor = sensors_[detector_name];

        // Loop over all the hits
        for(const auto& hit : hit_msg->getData()) {
            if(sensor_data::kMaxHits <= sensor.nhits_) {
                LOG(ERROR) << "More than " << sensor_data::kMaxHits << " in detector " << detector_name;
                continue;
            }

            // Fill the tree with received messages
            auto i = static_cast<size_t>(sensor.nhits_);
            sensor.pix_x_[i] = hit.getPixel().getIndex().x();
            sensor.pix_y_[i] = hit.getPixel().getIndex().y();
            sensor.value_[i] = static_cast<Int_t>(hit.getSignal());
            // Assumes that time is correctly digitized
            sensor.timing_[i] = static_cast<Int_t>(hit.getLocalTime());
            // This contains no useful information but it expected to be present
            sensor.hit_in_cluster_[i] = 0;
            sensor.nhits_ += 1;

            LOG(TRACE) << detector_name << " x=" << hit.getPixel().getIndex().x() << " y=" << hit.getPixel().getIndex().y()
                       << " t=" << hit.getLocalTime() << " signal=" << hit.getSignal();
        }
    }

    // Loop over all the detectors to fill all corresponding sensor trees
    for(auto& item : sensors_) {
        item.second.tree->Fill();
        LOG(TRACE) << "Wrote sensor event data for " << item.first;
    }
}

void RCEWriterModule::finalize() {
    output_file_->Write();
    LOG(TRACE) << "Wrote data to file";
}
