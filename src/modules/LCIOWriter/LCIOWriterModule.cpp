/**
 * @file
 * @brief Implementation of [LCIOWriter] module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "LCIOWriterModule.hpp"

#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

#include <Math/RotationZYX.h>

#include <IMPL/LCCollectionVec.h>
#include <IMPL/LCEventImpl.h>
#include <IMPL/LCRunHeaderImpl.h>
#include <IMPL/TrackImpl.h>
#include <IMPL/TrackerDataImpl.h>
#include <IMPL/TrackerHitImpl.h>
#include <IMPL/TrackerPulseImpl.h>
#include <IO/LCWriter.h>
#include <IOIMPL/LCFactory.h>
#include <UTIL/CellIDEncoder.h>
#include <lcio.h>

using namespace allpix;
using namespace lcio;

namespace eutelescope {
    static const std::string gTrackerHitEncoding = "sensorID:7,properties:7";
    static const std::string gTrackerPulseEncoding = "sensorID:7,xSeed:12,ySeed:12,xCluSize:5,yCluSize:5,type:5,quality:5";
    static const std::string gTrackerDataEncoding = "sensorID:7,sparsePixelType:5";

    enum HitProperties { kHitInGlobalCoord = 1L << 0, kFittedHit = 1L << 1, kSimulatedHit = 1L << 2, kDeltaHit = 1L << 3 };
    enum ClusterType {
        kEUTelFFClusterImpl = 0,
        kEUTelSparseClusterImpl = 1,
        kEUTelDFFClusterImpl = 2,
        kEUTelBrickedClusterImpl = 3,
        kEUTelGenericSparseClusterImpl = 4,
        kUnknown = 31
    };

} // namespace eutelescope

inline std::array<long double, 3> getRotationAnglesFromMatrix(ROOT::Math::Rotation3D const& rot_mat) {
    double r00 = 0;
    double r01 = 0;
    double r02 = 0;
    double r10 = 0;
    double r11 = 0;
    double r12 = 0;
    double r20 = 0;
    double r21 = 0;
    double r22 = 0;

    rot_mat.GetComponents(r00, r01, r02, r10, r11, r12, r20, r21, r22);

    long double aX = 0;
    long double aY = 0;
    long double aZ = 0;

    // This is a correct decomposition for the given rotation order: YXZ, i.e. initial Z-rotation, followed by X and
    // ultimately Y rotation. In the case of a gimbal lock, the angle around the Z axis is (arbitrarily) set to 0.
    if(r12 < 1) {
        if(r12 > -1) {
            aX = std::asin(-r12);
            aY = std::atan2(r02, r22);
            aZ = std::atan2(r10, r11);
        } else /* r12 == -1 */ {
            aX = std::numbers::pi / 2;
            aY = -std::atan2(-r01, r00);
            aZ = 0;
        }
    } else /* r12 == 1 */ {
        aX = -std::numbers::pi / 2;
        aY = std::atan2(-r01, r00);
        aZ = 0;
    }
    std::array<long double, 3> vec = {{aX, aY, aZ}};
    return vec;
}

LCIOWriterModule::LCIOWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo)
    : SequentialModule(config), messenger_(messenger), geo_mgr_(geo) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Set configuration defaults:
    config_.setDefault("file_name", "output.slcio");
    config_.setDefault("geometry_file", "allpix_squared_gear.xml");
    config_.setDefault("pixel_type", 2);
    config_.setDefault("detector_name", "EUTelescope");
    config_.setDefault("dump_mc_truth", false);

    pixel_type_ = config_.get<int>("pixel_type");
    detector_name_ = config_.get<std::string>("detector_name");
    dump_mc_truth_ = config_.get<bool>("dump_mc_truth");
    // There are two ways to configure this module - either by providing a "output_collection_name" or a
    // "detector_assignment". Throws an error if both are provided and defaults back to "output_collection_name" if none are
    // provided
    auto has_short_config = config_.has("output_collection_name");
    auto has_long_config = config_.has("detector_assignment");

    // Bind pixel hits message
    messenger_->bindMulti<PixelHitMessage>(this, MsgFlags::REQUIRED);
    messenger_->bindMulti<MCParticleMessage>(this, MsgFlags::REQUIRED);
    if(dump_mc_truth_) {
        messenger_->bindSingle<MCTrackMessage>(this, MsgFlags::REQUIRED);
    }

    if(has_short_config && has_long_config) {
        throw InvalidCombinationError(config_,
                                      {"output_collection_name", "detector_assignment"},
                                      "Provide either a \"output_collection_name\" or a \"detector_assignment\" "
                                      "configuration parameter. They are mutually exclusive!");
    } else if(!has_short_config && !has_long_config) {
        config_.setDefault("output_collection_name", "zsdata_m26");
        has_short_config = true;
    }

    auto detectors = geo_mgr_->getDetectors();

    if(has_short_config) {
        auto out_col_name = config_.get<std::string>("output_collection_name");
        unsigned sensor_id = 0;
        for(auto const& det : detectors) {
            auto const& det_name = det->getName();
            collections_to_detectors_map_[out_col_name].emplace_back(det_name);
            detector_names_to_id_[det_name] = sensor_id++;
        }
    } else {

        // The 'setup' parameter has a string matrix with three elements per row
        // ["detector_name", "output_collection", "sensor_id"] where the detector_name
        // must correspond to the detector name in the geometry file, the output_collection
        // will be the name of the lcio output collection (multiple detectors can write to
        // the same collection), and sensor_id has to be a unique id which the data corresponding
        // to this sensor will carry
        auto setup = config.getMatrix<std::string>("detector_assignment");
        auto assigned_ids = std::vector<unsigned>{};

        for(auto const& setup_entry : setup) {
            if(setup_entry.size() == 3) {
                auto const& det_name = setup_entry[0];
                auto const& col_name = setup_entry[1];
                auto const& sensor_id_str = setup_entry[2];
                // This map will help determine how many setup we will create (keys) and what
                // detectors write into that collection (values)
                collections_to_detectors_map_[col_name].emplace_back(det_name);

                unsigned sensor_id = 0;
                try {
                    auto sensor_id_unchecked = std::stoi(sensor_id_str);
                    if(sensor_id_unchecked >= 0 && sensor_id_unchecked <= 127) {
                        sensor_id = static_cast<unsigned>(sensor_id_unchecked);
                    } else {
                        throw InvalidValueError(config_,
                                                "detector_assignment",
                                                "The sensor id \"" + std::to_string(sensor_id_unchecked) +
                                                    "\" which was provided for detector \"" + det_name +
                                                    "\" must be positive and less than or equal to 127 (7 bit)");
                    }
                } catch(const std::invalid_argument&) {
                    throw InvalidValueError(config_,
                                            "detector_assignment",
                                            "The sensor id \"" + sensor_id_str +          // NOLINT
                                                "\" which was provided for detector \"" + // NOLINT
                                                det_name +                                // NOLINT
                                                "\" is not a valid integer");             // NOLINT
                }

                if(std::ranges::find(assigned_ids, sensor_id) == assigned_ids.end()) {
                    assigned_ids.emplace_back(sensor_id);
                    // This map will translate the internally used detector name to the sensor id
                    detector_names_to_id_[det_name] = sensor_id;
                } else {
                    throw InvalidValueError(config_,
                                            "detector_assignment",
                                            "Trying to assign sensor id \"" + std::to_string(sensor_id) +
                                                "\" to detector \"" + det_name + "\", this id is already assigned");
                }

            } else {
                auto error = std::string("The entry: [");
                for(auto const& value : setup_entry) {
                    error.append("\"" + value + "\", ");
                }
                error.pop_back();
                error.pop_back();
                error.append("] should have three entries in following order: [\"detector_name\", \"output_collection\", "
                             "\"sensor_id\"]");
                throw InvalidValueError(config_, "detector_assignment", error);
            }
        }

        // Cross check the detector geometry against the configuration file
        if(setup.size() != detectors.size()) {
            throw InvalidValueError(config_,
                                    "detector_assignment",
                                    "In the configuration file " + std::to_string(setup.size()) +
                                        " detectors are specified, in the geometry " + std::to_string(detectors.size()) +
                                        ", this is a mismatch");
        }
    }
    //
    for(auto const& col_dets_pair : collections_to_detectors_map_) {
        collection_names_vector_.emplace_back(col_dets_pair.first);
        LOG(DEBUG) << "Registered output collection \"" << col_dets_pair.first << "\" for sensors: ";
        for(auto const& det_name : col_dets_pair.second) {
            LOG(DEBUG) << det_name << " ";
            auto det_id = detector_names_to_id_[det_name];
            detector_ids_to_colllection_index_[det_id] = collection_names_vector_.size() - 1;
        }
    }

    for(const auto& det : detectors) {
        auto const& det_name = det->getName();
        auto it = detector_names_to_id_.find(det_name);
        if(it != detector_names_to_id_.end()) {
            LOG(DEBUG) << det_name << " has ID " << detector_names_to_id_[det_name];
        } else {
            throw InvalidValueError(config_,
                                    "detector_assignment",
                                    "Detector \"" + det_name +
                                        "\" is specified in the geometry file, but not provided in the configuration file");
        }
    }
}

void LCIOWriterModule::initialize() {
    // Create the output GEAR file for the detector geometry
    geometry_file_name_ = createOutputFile(config_.get<std::string>("geometry_file"), "xml");
    // Open LCIO file and write run header
    lcio_file_name_ = createOutputFile(config_.get<std::string>("file_name"), "slcio");
    lc_writer_ = std::shared_ptr<IO::LCWriter>(LCFactory::getInstance()->createLCWriter());
    lc_writer_->open(lcio_file_name_, LCIO::WRITE_NEW);
    auto run = std::make_unique<LCRunHeaderImpl>();
    run->setRunNumber(1);
    run->setDetectorName(detector_name_);
    lc_writer_->writeRunHeader(run.get());
}

void LCIOWriterModule::run(Event* event) {
    auto pixel_messages = messenger_->fetchMultiMessage<PixelHitMessage>(this, event);

    auto evt = std::make_unique<LCEventImpl>(); // create the event
    evt->setRunNumber(1);
    evt->setEventNumber(static_cast<int>(event->number)); // set the event attributes
    evt->parameters().setValue("EventType", 2);

    auto output_col_vec = std::vector<LCCollectionVec*>();
    auto output_col_encoder_vec = std::vector<std::unique_ptr<CellIDEncoder<TrackerDataImpl>>>();
    // Prepare dynamic output setup and their CellIDEncoders which are defined by the user's config
    for(size_t i = 0; i < collection_names_vector_.size(); ++i) {
        output_col_vec.emplace_back(new LCCollectionVec(LCIO::TRACKERDATA)); // NOLINT(cppcoreguidelines-owning-memory)
        output_col_encoder_vec.emplace_back(
            std::make_unique<CellIDEncoder<TrackerDataImpl>>(eutelescope::gTrackerDataEncoding, output_col_vec.back()));
    }

    LCCollectionVec* mc_cluster_vec = nullptr;
    LCCollectionVec* mc_cluster_raw_vec = nullptr;
    LCCollectionVec* mc_hit_vec = nullptr;
    LCCollectionVec* mc_track_vec = nullptr;

    std::unique_ptr<CellIDEncoder<TrackerDataImpl>> mc_cluster_raw_encoder = nullptr;
    std::unique_ptr<CellIDEncoder<TrackerPulseImpl>> mc_cluster_encoder = nullptr;
    std::unique_ptr<CellIDEncoder<TrackerHitImpl>> mc_hit_encoder = nullptr;

    // The detector id is only attached to the message, not the MCParticle, thus we store it here
    auto mcp_to_det_id = std::map<MCParticle const*, unsigned>{};
    // Multiple pixel hits can be assigned to a single MCParticle, here we store them in a LCIO 'float vector' to create the
    // Monte Carlo truth cluster
    auto mcp_to_pixel_data_vec = std::map<MCParticle const*, std::vector<std::vector<float>>>{};

    if(dump_mc_truth_) {
        // Prepare static Monte-Carlo output setup and their CellIDEncoders which are the same every time
        // NOLINTBEGIN(cppcoreguidelines-owning-memory)
        mc_cluster_vec = new LCCollectionVec(LCIO::TRACKERPULSE);
        mc_cluster_raw_vec = new LCCollectionVec(LCIO::TRACKERDATA);
        mc_hit_vec = new LCCollectionVec(LCIO::TRACKERHIT);
        mc_track_vec = new LCCollectionVec(LCIO::TRACK);
        // NOLINTEND(cppcoreguidelines-owning-memory)

        mc_cluster_raw_encoder =
            std::make_unique<CellIDEncoder<TrackerDataImpl>>(eutelescope::gTrackerDataEncoding, mc_cluster_raw_vec);
        mc_cluster_encoder =
            std::make_unique<CellIDEncoder<TrackerPulseImpl>>(eutelescope::gTrackerPulseEncoding, mc_cluster_vec);
        mc_hit_encoder = std::make_unique<CellIDEncoder<TrackerHitImpl>>(eutelescope::gTrackerHitEncoding, mc_hit_vec);
    }

    // In LCIO the 'charge vector' is a vector of floats which correspond to hit pixels, depending on the pixel
    // type in EUTelescope the number of entries per pixel varies
    std::map<unsigned, std::vector<float>> charges;
    for(auto const& det : detector_names_to_id_) {
        charges[det.second] = std::vector<float>{};
    }

    // Receive all pixel messages, fill charge vectors
    for(const auto& hit_msg : pixel_messages) {
        LOG(DEBUG) << hit_msg->getDetector()->getName();
        for(const auto& hitdata : hit_msg->getData()) {

            auto this_hit_charge_vec = std::vector<float>{};

            LOG(DEBUG) << "X: " << hitdata.getPixel().getIndex().x() << ", Y:" << hitdata.getPixel().getIndex().y()
                       << ", Signal: " << hitdata.getSignal();

            const auto det_id = detector_names_to_id_[hit_msg->getDetector()->getName()];

            switch(pixel_type_) {
            case 1:                                                                               // EUTelSimpleSparsePixel
                charges[det_id].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[det_id].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[det_id].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getSignal()));               // signal
                break;
            case 2:  // EUTelGenericSparsePixel
            default: // EUTelGenericSparsePixel is default
                charges[det_id].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[det_id].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[det_id].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                charges[det_id].push_back(0.0);
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getSignal()));               // signal
                this_hit_charge_vec.push_back(0.0);                                                   // time
                break;
            case 5:                                                                               // EUTelTimepix3SparsePixel
                charges[det_id].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[det_id].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[det_id].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                charges[det_id].push_back(0.0);                                                   // time
                charges[det_id].push_back(0.0);                                                   // time
                charges[det_id].push_back(0.0);                                                   // time
                charges[det_id].push_back(0.0);                                                   // time
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                this_hit_charge_vec.push_back(static_cast<float>(hitdata.getSignal()));               // signal
                this_hit_charge_vec.push_back(0.0);                                                   // time
                this_hit_charge_vec.push_back(0.0);                                                   // time
                this_hit_charge_vec.push_back(0.0);                                                   // time
                this_hit_charge_vec.push_back(0.0);                                                   // time
                break;
            }

            for(auto const& mcp : hitdata.getMCParticles()) {
                mcp_to_det_id[mcp] = det_id;
                mcp_to_pixel_data_vec[mcp].emplace_back(this_hit_charge_vec);
            }
        }
    }

    // Every track will be linked to at least one (typically multiple) MCParticles and thus TrackerData objects
    auto mctrk_to_hit_data_vec = std::map<MCTrack const*, std::vector<TrackerHitImpl*>>{};

    // A MCParticle will be reflected by an LCIO hit and cluster - the hit is stored in a TrackerHit, the cluster in
    // a TrackerPulse linked to a TrackerData object
    if(dump_mc_truth_) {
        for(auto& mcp_pixel_data_vec_pair : mcp_to_pixel_data_vec) {
            // NOLINTBEGIN(cppcoreguidelines-owning-memory)
            auto* mc_tracker_data = new TrackerDataImpl();
            auto* mc_tracker_pulse = new TrackerPulseImpl();
            auto* mc_tracker_hit = new TrackerHitImpl();
            // NOLINTEND(cppcoreguidelines-owning-memory)

            const auto& mc_particle = mcp_pixel_data_vec_pair.first;

            // Every detected pixel hit which had charge contribution from this MCParticle will be added to the cluster
            std::vector<float> truth_cluster_charge_vec;
            for(auto const& pixel_hit_charge_vec : mcp_pixel_data_vec_pair.second) {
                truth_cluster_charge_vec.insert(
                    std::end(truth_cluster_charge_vec), std::begin(pixel_hit_charge_vec), std::end(pixel_hit_charge_vec));
            }

            mc_tracker_data->setChargeValues(truth_cluster_charge_vec);
            (*mc_cluster_raw_encoder)["sensorID"] = mcp_to_det_id[mc_particle];
            (*mc_cluster_raw_encoder)["sparsePixelType"] = pixel_type_;
            mc_cluster_raw_encoder->setCellID(mc_tracker_data);
            mc_cluster_raw_vec->push_back(mc_tracker_data);

            mc_tracker_pulse->setTrackerData(mc_tracker_data);
            (*mc_cluster_encoder)["sensorID"] = mcp_to_det_id[mc_particle];
            (*mc_cluster_encoder)["type"] = 1; // corresponds to kEUTelGenericSparseClusterImpl
            mc_cluster_encoder->setCellID(mc_tracker_pulse);
            mc_cluster_vec->push_back(mc_tracker_pulse);

            // we take the centre of the MCParticle to be the global z-position
            auto const& hit_start_pos = mc_particle->getGlobalStartPoint();
            auto const& hit_end_pos = mc_particle->getGlobalEndPoint();
            auto pos_arr = std::array<double, 3>{{0.5 * (hit_start_pos.x() + hit_end_pos.x()),
                                                  0.5 * (hit_start_pos.y() + hit_end_pos.y()),
                                                  0.5 * (hit_start_pos.z() + hit_end_pos.z())}};
            mc_tracker_hit->setPosition(pos_arr.data());
            mc_tracker_hit->setType(1); // corresponds to kEUTelGenericSparseClusterImpl
            (*mc_hit_encoder)["sensorID"] = mcp_to_det_id[mc_particle];

            int hit_properties = eutelescope::HitProperties::kHitInGlobalCoord + eutelescope::HitProperties::kSimulatedHit;
            if(mc_particle->getTrack()->getParent() != nullptr) {
                hit_properties += eutelescope::HitProperties::kDeltaHit;
            }
            (*mc_hit_encoder)["properties"] = hit_properties;

            mc_hit_encoder->setCellID(mc_tracker_hit);
            mc_tracker_hit->rawHits() = std::vector<LCObject*>{mc_tracker_data};
            mc_hit_vec->push_back(mc_tracker_hit);
            mctrk_to_hit_data_vec[mc_particle->getTrack()].emplace_back(mc_tracker_hit);
        }
    }
    // Fill hitvector with event data
    for(auto const& det_id_name_pair : detector_names_to_id_) {
        auto det_id = det_id_name_pair.second;
        auto* hit = new TrackerDataImpl(); // NOLINT(cppcoreguidelines-owning-memory)
        hit->setChargeValues(charges[det_id]);
        auto col_index = detector_ids_to_colllection_index_[det_id];
        (*output_col_encoder_vec[col_index])["sensorID"] = det_id;
        (*output_col_encoder_vec[col_index])["sparsePixelType"] = pixel_type_;
        output_col_encoder_vec[col_index]->setCellID(hit);
        output_col_vec[col_index]->push_back(hit);
    }

    if(dump_mc_truth_) {
        LCFlagImpl flag(mc_track_vec->getFlag());
        flag.setBit(LCIO::TRBIT_HITS);
        mc_track_vec->setFlag(flag.getFlag());
        for(auto& pair : mctrk_to_hit_data_vec) {
            auto* track = new TrackImpl();
            for(auto& hit : pair.second) {
                // std::cout << "Got hit: " << hit << " z-pos: " << hit->getPosition()[2] << '\n';
                track->addHit(hit);
            }
            mc_track_vec->push_back(track);
        }

        // Add collection to event and write event to LCIO file
        evt->addCollection(mc_track_vec, "mc_track");
        evt->addCollection(mc_hit_vec, "mc_hit");
        evt->addCollection(mc_cluster_raw_vec, "mc_raw_cluster");
        evt->addCollection(mc_cluster_vec, "mc_cluster");
    }
    for(size_t i = 0; i < collection_names_vector_.size(); i++) {
        evt->addCollection(output_col_vec[i], collection_names_vector_[i]);
    }

    lc_writer_->writeEvent(evt.get()); // write the event to the file
    write_cnt_++;
}

void LCIOWriterModule::finalize() {
    lc_writer_->close();
    // Print statistics
    LOG(STATUS) << "Wrote " << write_cnt_ << " events to file:\n" << lcio_file_name_;

    // Write geometry:
    std::ofstream geometry_file;
    if(!geometry_file_name_.empty()) {
        geometry_file.open(geometry_file_name_, std::ios_base::out | std::ios_base::trunc);
        if(!geometry_file.good()) {
            throw ModuleError("Cannot write to GEAR geometry file");
        }

        auto detectors = geo_mgr_->getDetectors();
        geometry_file << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                      << "<!-- ?xml-stylesheet type=\"text/xsl\" href=\"https://cern.ch/allpix-squared/\"? -->\n"
                      << "<gear>\n";

        geometry_file << "  <global detectorName=\"" << detector_name_ << "\"/>\n";
        if(geo_mgr_->getMagneticFieldType() == MagneticFieldType::CONSTANT) {
            ROOT::Math::XYZVector b_field = geo_mgr_->getMagneticField(ROOT::Math::XYZPoint(0., 0., 0.));
            geometry_file << "  <BField type=\"ConstantBField\" x=\"" << Units::convert(b_field.x(), "T") << "\" y=\""
                          << Units::convert(b_field.y(), "T") << "\" z=\"" << Units::convert(b_field.z(), "T") << "\"/>\n";
        } else if(geo_mgr_->getMagneticFieldType() == MagneticFieldType::NONE) {
            geometry_file << "  <BField type=\"ConstantBField\" x=\"0.0\" y=\"0.0\" z=\"0.0\"/>\n";
        } else {
            LOG(WARNING) << "Field type not handled by GEAR geometry. Writing null magnetic field instead.";
            geometry_file << "  <BField type=\"ConstantBField\" x=\"0.0\" y=\"0.0\" z=\"0.0\"/>\n";
        }

        geometry_file << "  <detectors>\n";
        geometry_file << "    <detector name=\"SiPlanes\" geartype=\"SiPlanesParameters\">\n";
        geometry_file << "      <siplanesType type=\"TelescopeWithoutDUT\"/>\n";
        geometry_file << "      <siplanesNumber number=\"" << detectors.size() << "\"/>\n";
        geometry_file << "      <siplanesID ID=\"" << 0 << "\"/>\n";
        geometry_file << "      <layers>\n";

        for(auto& detector : detectors) {
            // Write header for the layer:
            geometry_file << "      <!-- Allpix Squared Detector: " << detector->getName()
                          << " - type: " << detector->getType() << " -->\n";
            geometry_file << "        <layer>\n";

            auto position = detector->getPosition();

            auto model = detector->getModel();
            auto npixels = model->getNPixels();
            auto pitch = model->getPixelSize();

            auto total_size = model->getSize();
            auto sensitive_size = model->getSensorSize();

            // Write ladder
            geometry_file << "          <ladder ID=\"" << detector_names_to_id_[detector->getName()] << "\"\n";
            geometry_file << "            positionX=\"" << Units::convert(position.x(), "mm") << "\"\tpositionY=\""
                          << Units::convert(position.y(), "mm") << "\"\tpositionZ=\"" << Units::convert(position.z(), "mm")
                          << "\"\n";

            auto angles = getRotationAnglesFromMatrix(detector->getOrientation());

            geometry_file << "            rotationZY=\"" << Units::convert(-angles[0], "deg") << "\"     rotationZX=\""
                          << Units::convert(-angles[1], "deg") << "\"   rotationXY=\"" << Units::convert(-angles[2], "deg")
                          << "\"\n";
            geometry_file << "            sizeX=\"" << Units::convert(total_size.x(), "mm") << "\"\tsizeY=\""
                          << Units::convert(total_size.y(), "mm") << "\"\tthickness=\""
                          << Units::convert(total_size.z(), "mm") << "\"\n";
            geometry_file << "            radLength=\"93.65\"\n";
            geometry_file << "            />\n";

            // Write sensitive
            geometry_file << "          <sensitive ID=\"" << detector_names_to_id_[detector->getName()] << "\"\n";
            geometry_file << "            positionX=\"" << Units::convert(position.x(), "mm") << "\"\tpositionY=\""
                          << Units::convert(position.y(), "mm") << "\"\tpositionZ=\"" << Units::convert(position.z(), "mm")
                          << "\"\n";
            geometry_file << "            sizeX=\"" << Units::convert(npixels.x() * pitch.x(), "mm") << "\"\tsizeY=\""
                          << Units::convert(npixels.y() * pitch.y(), "mm") << "\"\tthickness=\""
                          << Units::convert(sensitive_size.z(), "mm") << "\"\n";
            geometry_file << "            npixelX=\"" << npixels.x() << "\"\tnpixelY=\"" << npixels.y() << "\"\n";
            geometry_file << "            pitchX=\"" << Units::convert(pitch.x(), "mm") << "\"\tpitchY=\""
                          << Units::convert(pitch.y(), "mm") << "\"\tresolution=\""
                          << Units::convert(pitch.x() / std::sqrt(12), "mm") << "\"\n";
            geometry_file << "            rotation1=\"1.0\"\trotation2=\"0.0\"\n";
            geometry_file << "            rotation3=\"0.0\"\trotation4=\"1.0\"\n";
            geometry_file << "            radLength=\"93.65\"\n";
            geometry_file << "            />\n";

            // End the layer:
            geometry_file << "        </layer>\n";
        }

        // Close XML tree:
        geometry_file << "      </layers>\n"
                      << "    </detector>\n"
                      << "  </detectors>\n"
                      << "</gear>\n";

        LOG(STATUS) << "Wrote GEAR geometry to file:\n" << geometry_file_name_;
    }
}
