/**
 * @file
 * @brief Implementation of RCE Writer Module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "RCEWriterModule.hpp"

#include <cassert>
#include <fstream>
#include <utility>

#include <TBranchElement.h>
#include <TClass.h>
#include <TDecompSVD.h>
#include <TDirectory.h>
#include <TMatrixD.h>

#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "objects/Object.hpp"
#include "objects/objects.h"

using namespace allpix;

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

    // we need to restore the ostream format state later on
    auto flags = os.flags();
    auto precision = os.precision();
    os << std::showpoint;
    os << std::setprecision(std::numeric_limits<double>::max_digits10);

    os << "[[sensors]]\n";
    os << "id = " << index << '\n';
    os << "offset = [" << offset.x() << ", " << offset.y() << ", " << offset.z() << "]\n";
    os << "unit_u = [" << unit_u.x() << ", " << unit_u.y() << ", " << unit_u.z() << "]\n";
    os << "unit_v = [" << unit_v.x() << ", " << unit_v.y() << ", " << unit_v.z() << "]\n";
    os << '\n';

    os.flags(flags);
    os.precision(precision);
}

static void print_geometry(std::ostream& os, const std::vector<std::string>& names, GeometryManager& geo_mgr) {
    int index = 0;
    for(const auto& name : names) {
        print_geometry_sensor(os, index++, *geo_mgr.getDetector(name));
    }
}

RCEWriterModule::RCEWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_mgr)
    : Module(config), geo_mgr_(geo_mgr) {
    assert(geo_mgr_ and "GeometryManager must be non-null");

    // Bind to PixelHitMessage
    messenger->bindMulti(this, &RCEWriterModule::pixel_hit_messages_);

    config_.setDefault("file_name", "rce-data.root");
    config_.setDefault("geometry_file", "rce-geo.toml");
}

void RCEWriterModule::init() {
    // We need a sorted list of names to assign monotonic, numeric ids
    std::vector<std::string> detector_names;
    for(const auto& detector : geo_mgr_->getDetectors()) {
        detector_names.push_back(detector->getName());
    }
    std::sort(detector_names.begin(), detector_names.end());

    // Open output data file
    std::string path_data = createOutputFile(add_file_extension(config_.get<std::string>("file_name"), "root"));
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

    // Write proteus geometry file
    std::string path_geo = createOutputFile(add_file_extension(config_.get<std::string>("geometry_file"), "toml"));
    std::ofstream geo_file(path_geo);
    print_geometry(geo_file, detector_names, *geo_mgr_);
}

void RCEWriterModule::run(unsigned int event_id) {
    // fill per-event data
    timestamp_ = 0;
    frame_number_ = event_id;
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
    for(const auto& hit_msg : pixel_hit_messages_) {
        const auto& detector_name = hit_msg->getDetector()->getName();
        auto& sensor = sensors_[detector_name];

        // Loop over all the hits
        for(const auto& hit : hit_msg->getData()) {
            if(sensor.kMaxHits <= sensor.nhits_) {
                LOG(ERROR) << "More than " << sensor.kMaxHits << " in detector " << detector_name;
                continue;
            }

            // Fill the tree with received messages
            auto i = sensor.nhits_;
            sensor.pix_x_[i] = static_cast<Int_t>(hit.getPixel().getIndex().x()); // NOLINT
            sensor.pix_y_[i] = static_cast<Int_t>(hit.getPixel().getIndex().y()); // NOLINT
            sensor.value_[i] = static_cast<Int_t>(hit.getSignal());               // NOLINT
            // Assumes that time is correctly digitized
            sensor.timing_[i] = static_cast<Int_t>(hit.getTime()); // NOLINT
            // This contains no useful information but it expected to be present
            sensor.hit_in_cluster_[i] = 0; // NOLINT
            sensor.nhits_ += 1;

            LOG(TRACE) << detector_name << " x=" << hit.getPixel().getIndex().x() << " y=" << hit.getPixel().getIndex().y()
                       << " t=" << hit.getTime() << " signal=" << hit.getSignal();
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
