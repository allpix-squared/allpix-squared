/**
 * @file
 * @brief Implementation of module to read electric fields
 * @copyright MIT License
 */

#include "ElectricFieldReaderModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/Vector3D.h>
#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

ElectricFieldReaderModule::ElectricFieldReaderModule(Configuration config, Messenger*, std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), detector_(std::move(detector)) {}

void ElectricFieldReaderModule::init() {
    // Get type of electric field from model parameter
    ElectricFieldReaderModule::FieldData field_data;
    if(config_.get<std::string>("model") == "linear") {
        field_data = construct_linear_field();
    } else if(config_.get<std::string>("model") == "init") {
        field_data = read_init_field();
    } else {
        throw InvalidValueError(config_, "model", "model should be 'linear' or 'init'");
    }

    // Set detector field
    detector_->setElectricField(field_data.first, field_data.second);

    // Produce histograms if needed
    if(config_.get<bool>("output_plots", false)) {
        create_output_plots();
    }
}

/**
 * The linear field does not have a X or Y component and is constant over the whole thickness of the sensor.
 */
ElectricFieldReaderModule::FieldData ElectricFieldReaderModule::construct_linear_field() {
    LOG(TRACE) << "Constructing electric field from linear bias voltage";

    // Compute the electric field
    LOG(DEBUG) << getDetector()->getModel()->getSensorSize().z();
    auto field_z = config_.get<double>("voltage") / getDetector()->getModel()->getSensorSize().z();
    LOG(INFO) << "Set linear electric field with magnitude " << Units::display(field_z, {"V/um", "V/mm"});

    // Create the field vector
    auto field = std::make_shared<std::vector<double>>(3);
    (*field)[0] = 0;
    (*field)[1] = 0;
    (*field)[2] = field_z;

    // Return the constructed field
    return ElectricFieldReaderModule::FieldData(field, {{1, 1, 1}});
}

/**
 * The field read from the INIT format are shared between module instantiations using the static
 * ElectricFieldReaderModuleget_by_file_name method.
 */
ElectricFieldReaderModule::FieldData ElectricFieldReaderModule::read_init_field() {
    try {
        LOG(TRACE) << "Fetching electric field from init file";

        // Get field from file
        auto field_data = get_by_file_name(config_.getPath("file_name", true), *detector_.get());
        LOG(INFO) << "Set electric field with " << field_data.second.at(0) << "x" << field_data.second.at(1) << "x"
                  << field_data.second.at(2) << " cells";

        // Return the field data
        return field_data;
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::runtime_error& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::bad_alloc& e) {
        throw InvalidValueError(config_, "file_name", "file too large");
    }
}

void ElectricFieldReaderModule::create_output_plots() {
    LOG(TRACE) << "Creating output plots";

    auto steps = config_.get<size_t>("output_plots_steps", 500);
    auto project = config_.get<char>("output_plots_project", 'x');

    if(project != 'x' && project != 'y' && project != 'z') {
        throw InvalidValueError(config_, "output_plots_project", "can only project on x, y or z axis");
    }

    auto model = detector_->getModel();
    if(config_.get<bool>("output_plots_single_pixel", false)) {
        // If we need to plot a single pixel we change the model to fake the sensor to be a single pixel
        // NOTE: This is a little hacky, but is the easiest approach
        model = std::make_shared<DetectorModel>(*model);
        model->setSensorExcessTop(0);
        model->setSensorExcessBottom(0);
        model->setSensorExcessLeft(0);
        model->setSensorExcessRight(0);
        model->setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>(1, 1));
    }

    // Determine minimum and maximum index depending on projection axis
    double min1, max1;
    double min2, max2;
    if(project == 'x') {
        min1 = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0;
        max1 = model->getSensorCenter().y() + model->getSensorSize().y() / 2.0;
        min2 = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0;
        max2 = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    } else if(project == 'y') {
        min1 = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0;
        max1 = model->getSensorCenter().x() + model->getSensorSize().x() / 2.0;
        min2 = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0;
        max2 = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    } else {
        min1 = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0;
        max1 = model->getSensorCenter().x() + model->getSensorSize().x() / 2.0;
        min2 = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0;
        max2 = model->getSensorCenter().y() + model->getSensorSize().y() / 2.0;
    }

    // Create 2D histogram
    auto histogram = new TH2F("field",
                              ("Electric field for " + detector_->getName()).c_str(),
                              static_cast<int>(steps),
                              min1,
                              max1,
                              static_cast<int>(steps),
                              min2,
                              max2);

    // Determine the coordinate to use for projection
    double x = 0, y = 0, z = 0;
    if(project == 'x') {
        x = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0 +
            config_.get<double>("output_plots_projection_percentage", 0.5) * model->getSensorSize().x();
    } else if(project == 'y') {
        y = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0 +
            config_.get<double>("output_plots_projection_percentage", 0.5) * model->getSensorSize().y();
    } else {
        z = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0 +
            config_.get<double>("output_plots_projection_percentage", 0.5) * model->getSensorSize().z();
    }

    // Find the electric field at every index
    for(size_t j = 0; j < steps; ++j) {
        if(project == 'x') {
            y = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().y();
        } else if(project == 'y') {
            x = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().x();
        } else {
            x = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().x();
        }
        for(size_t k = 0; k < steps; ++k) {
            if(project == 'x') {
                z = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0 +
                    ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().z();
            } else if(project == 'y') {
                z = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0 +
                    ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().z();
            } else {
                y = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0 +
                    ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().y();
            }

            // Get field strength from detector
            auto field_strength = std::sqrt(detector_->getElectricField(ROOT::Math::XYZPoint(x, y, z)).Mag2());

            // Fill the histogram
            if(project == 'x') {
                histogram->Fill(y, z, field_strength);
            } else if(project == 'y') {
                histogram->Fill(x, z, field_strength);
            } else {
                histogram->Fill(x, y, field_strength);
            }
        }
    }

    // Write the histogram to module file
    histogram->Write();
}

/**
 * @brief Check if the detector matches the file header
 */
inline static void check_detector_match(Detector& detector, double thickness, double xpixsz, double ypixsz) {
    auto model = detector.getModel();
    // Do a several checks with the detector model
    if(model != nullptr) {
        if(std::fabs(thickness - model->getSensorSize().z()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Thickness of sensor in file is " << Units::display(thickness, "um")
                         << " but in the model it is " << Units::display(model->getSensorSize().z(), "um");
        }
        if(std::fabs(xpixsz - model->getPixelSize().x()) > std::numeric_limits<double>::epsilon() ||
           std::fabs(ypixsz - model->getPixelSize().y()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Pixel size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"}) << ") but in the model it is ("
                         << Units::display(model->getPixelSize().x(), {"um", "mm"}) << ","
                         << Units::display(model->getPixelSize().y(), {"um", "mm"}) << ")";
        }
    }
}

std::map<std::string, ElectricFieldReaderModule::FieldData> ElectricFieldReaderModule::field_map_;
ElectricFieldReaderModule::FieldData ElectricFieldReaderModule::get_by_file_name(const std::string& file_name,
                                                                                 Detector& detector) {
    // Search in cache (NOTE: the path reached here is always a canonical name)
    auto iter = field_map_.find(file_name);
    if(iter != field_map_.end()) {
        // FIXME Check detector match here as well
        return iter->second;
    }

    // Load file
    std::ifstream file(file_name);

    std::string header;
    std::getline(file, header);

    LOG(TRACE) << "Header of file " << file_name << " is " << header;

    // Read the header
    std::string tmp;
    file >> tmp >> tmp;        // ignore the init seed and cluster length
    file >> tmp >> tmp >> tmp; // ignore the incident pion direction
    file >> tmp >> tmp >> tmp; // ignore the magnetic field (specify separately)
    double thickness, xpixsz, ypixsz;
    file >> thickness >> xpixsz >> ypixsz;
    thickness = Units::get(thickness, "um");
    xpixsz = Units::get(xpixsz, "um");
    ypixsz = Units::get(ypixsz, "um");
    file >> tmp >> tmp >> tmp >> tmp; // ignore temperature, flux, rhe (?) and new_drde (?)
    size_t xsize, ysize, zsize;
    file >> xsize >> ysize >> zsize;
    file >> tmp;

    // Check if electric field matches chip
    check_detector_match(detector, thickness, xpixsz, ypixsz);

    if(file.fail()) {
        throw std::runtime_error("invalid data or unexpected end of file");
    }
    auto field = std::make_shared<std::vector<double>>();
    field->resize(xsize * ysize * zsize * 3);

    // Loop through all the field data
    for(size_t i = 0; i < xsize * ysize * zsize; ++i) {
        if(file.eof()) {
            throw std::runtime_error("unexpected end of file");
        }

        // Get index of electric field
        size_t xind, yind, zind;
        file >> xind >> yind >> zind;

        if(file.fail() || xind > xsize || yind > ysize || zind > zsize) {
            throw std::runtime_error("invalid data");
        }
        xind--;
        yind--;
        zind--;

        // Loop through components of electric field
        for(size_t j = 0; j < 3; ++j) {
            double input;
            file >> input;

            // Set the electric field at a position
            (*field)[xind * ysize * zsize * 3 + yind * zsize * 3 + zind * 3 + j] = Units::get(input, "V/cm");
        }
    }

    return std::make_pair(field, std::array<size_t, 3>{{xsize, ysize, zsize}});
}
