/**
 * @file
 * @brief Implementation of [LCIOWriter] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "LCIOWriterModule.hpp"

#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "core/messenger/Messenger.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"

#include <Math/RotationZYX.h>

#include <IMPL/LCCollectionVec.h>
#include <IMPL/LCEventImpl.h>
#include <IMPL/LCRunHeaderImpl.h>
#include <IMPL/TrackerDataImpl.h>
#include <IO/LCWriter.h>
#include <IOIMPL/LCFactory.h>
#include <UTIL/CellIDEncoder.h>
#include <lcio.h>

using namespace allpix;
using namespace lcio;

LCIOWriterModule::LCIOWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo)
    : Module(config), geo_mgr_(geo) {

    // Bind pixel hits message
    messenger->bindMulti(this, &LCIOWriterModule::pixel_messages_, MsgFlags::REQUIRED);

    // get all detector names and assign id.
    auto detectors = geo_mgr_->getDetectors();
    unsigned int i = 0;
    for(const auto& det : detectors) {
        detectorIDs_[det->getName()] = i;
        LOG(DEBUG) << det->getName() << " has ID " << detectorIDs_[det->getName()];
        i++;
    }

    // Set configuration defaults:
    config_.setDefault("file_name", "output.slcio");
    config_.setDefault("geometry_file", "allpix_squared_gear.xml");
    config_.setDefault("pixel_type", 2);
    config_.setDefault("detector_name", "EUTelescope");
    config_.setDefault("output_collection_name", "zsdata_m26");

    pixelType_ = config_.get<int>("pixel_type");
    DetectorName_ = config_.get<std::string>("detector_name");
    OutputCollectionName_ = config_.get<std::string>("output_collection_name");
}

void LCIOWriterModule::init() {
    // Create the output GEAR file for the detector geometry
    geometry_file_name_ = createOutputFile(allpix::add_file_extension(config_.get<std::string>("geometry_file"), "xml"));

    // Open LCIO file and write run header
    lcio_file_name_ = createOutputFile(allpix::add_file_extension(config_.get<std::string>("file_name"), "slcio"));
    lcWriter_ = std::shared_ptr<IO::LCWriter>(LCFactory::getInstance()->createLCWriter());
    lcWriter_->open(lcio_file_name_, LCIO::WRITE_NEW);
    auto run = std::make_unique<LCRunHeaderImpl>();
    run->setRunNumber(1);
    run->setDetectorName(DetectorName_);
    lcWriter_->writeRunHeader(run.get());
}

void LCIOWriterModule::run(unsigned int eventNb) {

    auto evt = std::make_unique<LCEventImpl>(); // create the event
    evt->setRunNumber(1);
    evt->setEventNumber(static_cast<int>(eventNb)); // set the event attributes
    evt->parameters().setValue("EventType", 2);

    // Prepare charge vectors
    std::vector<std::vector<float>> charges;
    for(unsigned int i = 0; i < detectorIDs_.size(); i++) {
        std::vector<float> charge;
        charges.push_back(charge);
    }

    // Receive all pixel messages, fill charge vectors
    for(const auto& hit_msg : pixel_messages_) {
        LOG(DEBUG) << hit_msg->getDetector()->getName();
        for(const auto& hitdata : hit_msg->getData()) {
            LOG(DEBUG) << "X: " << hitdata.getPixel().getIndex().x() << ", Y:" << hitdata.getPixel().getIndex().y()
                       << ", Signal: " << hitdata.getSignal();

            unsigned int detectorID = detectorIDs_[hit_msg->getDetector()->getName()];

            switch(pixelType_) {
            case 1: // EUTelSimpleSparsePixel
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[detectorID].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                break;
            case 2:  // EUTelGenericSparsePixel
            default: // EUTelGenericSparsePixel is default
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[detectorID].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                break;
            case 5: // EUTelTimepix3SparsePixel
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().x())); // x
                charges[detectorID].push_back(static_cast<float>(hitdata.getPixel().getIndex().y())); // y
                charges[detectorID].push_back(static_cast<float>(hitdata.getSignal()));               // signal
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                charges[detectorID].push_back(static_cast<float>(0));                                 // time
                break;
            }
        }
    }

    // Prepare hitvector
    LCCollectionVec* hitVec = new LCCollectionVec(LCIO::TRACKERDATA);

    // Fill hitvector with event data
    for(unsigned int detectorID = 0; detectorID < detectorIDs_.size(); detectorID++) {
        auto hit = new TrackerDataImpl();
        CellIDEncoder<TrackerDataImpl> sparseDataEncoder("sensorID:7,sparsePixelType:5", hitVec);
        sparseDataEncoder["sensorID"] = detectorID;
        sparseDataEncoder["sparsePixelType"] = pixelType_;
        sparseDataEncoder.setCellID(hit);
        hit->setChargeValues(charges[detectorID]);
        hitVec->push_back(hit);
    }

    // Add collection to event and write event to LCIO file
    evt->addCollection(hitVec, OutputCollectionName_); // add the collection with a name
    lcWriter_->writeEvent(evt.get());                  // write the event to the file
    write_cnt_++;
}

void LCIOWriterModule::finalize() {
    lcWriter_->close();
    // Print statistics
    LOG(STATUS) << "Wrote " << write_cnt_ << " events to file:" << std::endl << lcio_file_name_;

    // Write geometry:
    std::ofstream geometry_file;
    if(!geometry_file_name_.empty()) {
        geometry_file.open(geometry_file_name_, std::ios_base::out | std::ios_base::trunc);
        if(!geometry_file.good()) {
            throw ModuleError("Cannot write to GEAR geometry file");
        }

        auto detectors = geo_mgr_->getDetectors();
        geometry_file << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl
                      << "<!-- ?xml-stylesheet type=\"text/xsl\" href=\"https://cern.ch/allpix-squared/\"? -->" << std::endl
                      << "<gear>" << std::endl;

        geometry_file << "  <global detectorName=\"" << DetectorName_ << "\"/>" << std::endl;
        if(geo_mgr_->getMagneticFieldType() == MagneticFieldType::CONSTANT) {
            ROOT::Math::XYZVector b_field = geo_mgr_->getMagneticField(ROOT::Math::XYZPoint(0., 0., 0.));
            geometry_file << "  <BField type=\"ConstantBField\" x=\"" << Units::convert(b_field.x(), "T") << "\" y=\""
                          << Units::convert(b_field.y(), "T") << "\" z=\"" << Units::convert(b_field.z(), "T") << "\"/>"
                          << std::endl;
        } else if(geo_mgr_->getMagneticFieldType() == MagneticFieldType::NONE) {
            geometry_file << "  <BField type=\"ConstantBField\" x=\"0.0\" y=\"0.0\" z=\"0.0\"/>" << std::endl;
        } else {
            LOG(WARNING) << "Field type not handled by GEAR geometry. Writing null magnetic field instead.";
            geometry_file << "  <BField type=\"ConstantBField\" x=\"0.0\" y=\"0.0\" z=\"0.0\"/>" << std::endl;
        }
        geometry_file << "  <detectors>" << std::endl;
        geometry_file << "    <detector name=\"SiPlanes\" geartype=\"SiPlanesParameters\">" << std::endl;
        geometry_file << "      <siplanesType type=\"TelescopeWithoutDUT\"/>" << std::endl;
        geometry_file << "      <siplanesNumber number=\"" << detectors.size() << "\"/>" << std::endl;
        geometry_file << "      <siplanesID ID=\"" << 0 << "\"/>" << std::endl;
        geometry_file << "      <layers>" << std::endl;

        for(auto& detector : detectors) {
            // Write header for the layer:
            geometry_file << "      <!-- Allpix Squared Detector: " << detector->getName()
                          << " - type: " << detector->getType() << " -->" << std::endl;
            geometry_file << "        <layer>" << std::endl;

            auto position = detector->getPosition();

            auto model = detector->getModel();
            auto npixels = model->getNPixels();
            auto pitch = model->getPixelSize();

            auto total_size = model->getSize();
            auto sensitive_size = model->getSensorSize();

            // Write ladder
            geometry_file << "          <ladder ID=\"" << detectorIDs_[detector->getName()] << "\"" << std::endl;
            geometry_file << "            positionX=\"" << Units::convert(position.x(), "mm") << "\"\tpositionY=\""
                          << Units::convert(position.y(), "mm") << "\"\tpositionZ=\"" << Units::convert(position.z(), "mm")
                          << "\"" << std::endl;

            // Use inverse ZYX rotation to retrieve XYZ angles as used in EUTelescope:
            ROOT::Math::RotationZYX rotations(detector->getOrientation().Inverse());
            geometry_file << "            rotationZY=\"" << Units::convert(-rotations.Psi(), "deg") << "\"     rotationZX=\""
                          << Units::convert(-rotations.Theta(), "deg") << "\"   rotationXY=\""
                          << Units::convert(-rotations.Phi(), "deg") << "\"" << std::endl;
            geometry_file << "            sizeX=\"" << Units::convert(total_size.x(), "mm") << "\"\tsizeY=\""
                          << Units::convert(total_size.y(), "mm") << "\"\tthickness=\""
                          << Units::convert(total_size.z(), "mm") << "\"" << std::endl;
            geometry_file << "            radLength=\"93.65\"" << std::endl;
            geometry_file << "            />" << std::endl;

            // Write sensitive
            geometry_file << "          <sensitive ID=\"" << detectorIDs_[detector->getName()] << "\"" << std::endl;
            geometry_file << "            positionX=\"" << Units::convert(position.x(), "mm") << "\"\tpositionY=\""
                          << Units::convert(position.y(), "mm") << "\"\tpositionZ=\"" << Units::convert(position.z(), "mm")
                          << "\"" << std::endl;
            geometry_file << "            sizeX=\"" << Units::convert(npixels.x() * pitch.x(), "mm") << "\"\tsizeY=\""
                          << Units::convert(npixels.y() * pitch.y(), "mm") << "\"\tthickness=\""
                          << Units::convert(sensitive_size.z(), "mm") << "\"" << std::endl;
            geometry_file << "            npixelX=\"" << npixels.x() << "\"\tnpixelY=\"" << npixels.y() << "\"" << std::endl;
            geometry_file << "            pitchX=\"" << Units::convert(pitch.x(), "mm") << "\"\tpitchY=\""
                          << Units::convert(pitch.y(), "mm") << "\"\tresolution=\""
                          << Units::convert(pitch.x() / std::sqrt(12), "mm") << "\"" << std::endl;
            geometry_file << "            rotation1=\"1.0\"\trotation2=\"0.0\"" << std::endl;
            geometry_file << "            rotation3=\"0.0\"\trotation4=\"1.0\"" << std::endl;
            geometry_file << "            radLength=\"93.65\"" << std::endl;
            geometry_file << "            />" << std::endl;

            // End the layer:
            geometry_file << "        </layer>" << std::endl;
        }

        // Close XML tree:
        geometry_file << "      </layers>" << std::endl
                      << "    </detector>" << std::endl
                      << "  </detectors>" << std::endl
                      << "</gear>" << std::endl;

        LOG(STATUS) << "Wrote GEAR geometry to file:" << std::endl << geometry_file_name_;
    }
}
