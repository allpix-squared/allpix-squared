/**
 * @file
 * @brief Implementation of module to read weighting fields
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied
 * verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities
 * granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "WeightingFieldReaderModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/RotationZ.h>
#include <Math/Vector3D.h>
#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

WeightingFieldReaderModule::WeightingFieldReaderModule(Configuration& config, Messenger*, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {}

void WeightingFieldReaderModule::init() {

    auto field_model = config_.get<std::string>("model");

    // Calculate thickness domain
    auto model = detector_->getModel();
    auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    auto thickness_domain = std::make_pair(sensor_max_z - model->getSensorSize().z(), sensor_max_z);

    // Calculate the field depending on the configuration
    if(field_model == "init") {
        WeightingFieldReaderModule::FieldData field_data;
        field_data = read_init_field();
        detector_->setWeightingFieldGrid(field_data.first, field_data.second, thickness_domain);
    } else if(field_model == "pad") {
        LOG(TRACE) << "Adding weighting field from pad in plane condenser";

        // Get pixel implant size from the detector model:
        auto implant = model->getImplantSize();
        ElectricFieldFunction function = get_pad_field_function(implant, thickness_domain);
        detector_->setWeightingFieldFunction(function, thickness_domain, WeightingFieldType::PAD);
    } else {
        throw InvalidValueError(config_, "model", "model should be 'init' or `pad`");
    }

    // Produce histograms if needed
    if(config_.get<bool>("output_plots", false)) {
        create_output_plots();
    }
}

/**
 * Implement weighting field from doi:10.1016/j.nima.2014.08.44 and return it as a lookup function.
 */
ElectricFieldFunction WeightingFieldReaderModule::get_pad_field_function(const ROOT::Math::XYVector& implant,
                                                                         std::pair<double, double> thickness_domain) {

    LOG(TRACE) << "Calculating function for the plane condenser weighting field field." << std::endl;

    return [implant, thickness_domain](const ROOT::Math::XYZPoint& pos) {
        // Calculate values of the "g" function for all three components
        auto g = [implant](double x, double y, double u) {
            // Calculate terms derived from the arctan functions in the potential for all components
            auto fraction = [](double a, double b, double c) {
                double denominator = std::sqrt(a * a + b * b + c * c);
                // Three different components required since the derivatives have different solutions
                return ROOT::Math::XYZVector(b * c / ((a * a + c * c) * denominator),
                                             a * c / ((b * b + c * c) * denominator),
                                             a * b * (a * a + b * b + 2 * c * c) /
                                                 ((a * a + c * c) * (b * b + c * c) * denominator));
            };

            // Shift the x and y coordinates by plus/minus half the implant size:
            double x1 = x - implant.x() / 2;
            double x2 = x + implant.x() / 2;
            double y1 = y - implant.y() / 2;
            double y2 = y + implant.y() / 2;

            // Sum the four components of the "g" function and return:
            return (fraction(x1, y1, u) + fraction(x2, y2, u) - fraction(x1, y2, u) - fraction(x2, y1, u));
        };

        // Transform into coordinate system with sensor between d/2 < z < -d/2:
        auto d = thickness_domain.second - thickness_domain.first;
        auto local_z = -pos.z() + thickness_domain.second;

        // Calculate the series expansion
        ROOT::Math::XYZVector sum;
        for(int n = 1; n <= 100; n++) {
            sum += g(pos.x(), pos.y(), 2 * n * d - local_z) +
                   ROOT::Math::RotationZ(M_PI) * g(pos.x(), pos.y(), 2 * n * d + local_z);
        }

        return (1 / (2 * M_PI) * (ROOT::Math::RotationZ(M_PI) * g(pos.x(), pos.y(), local_z) + sum));
    };
}

void WeightingFieldReaderModule::create_output_plots() {
    LOG(TRACE) << "Creating output plots";

    auto steps = config_.get<size_t>("output_plots_steps", 500);
    auto position = config_.get<ROOT::Math::XYPoint>("output_plots_position", ROOT::Math::XYPoint(0, 0));

    auto model = detector_->getModel();

    double min = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0;
    double max = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;

    // Create 1D histograms
    auto histogramX =
        new TH1F("field1d_x", "E_{w}/V_{w} (x-component);x (mm);field strength (1/cm)", static_cast<int>(steps), min, max);
    auto histogramY =
        new TH1F("field1d_y", "E_{w}/V_{w} (y-component);y (mm);field strength (1/cm)", static_cast<int>(steps), min, max);
    auto histogramZ =
        new TH1F("field1d_z", "E_{w}/V_{w} (z-component);z (mm);field strength (1/cm)", static_cast<int>(steps), min, max);

    // Get the weighting field at every index
    for(size_t j = 0; j < steps; ++j) {
        double z = min + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * (max - min);

        // Get field strength from detector
        auto field = detector_->getWeightingField(ROOT::Math::XYZPoint(position.x(), position.y(), z), 0, 0);
        auto field_x_strength = Units::convert(field.x(), "1/cm");
        auto field_y_strength = Units::convert(field.y(), "1/cm");
        auto field_z_strength = Units::convert(field.z(), "1/cm");

        // Fill the histograms
        histogramX->Fill(z, static_cast<double>(field_x_strength));
        histogramY->Fill(z, static_cast<double>(field_y_strength));
        histogramZ->Fill(z, static_cast<double>(field_z_strength));
    }

    // Create 2D histogram
    auto histogram = new TH2F("field_z",
                              "E_{w}/V_{w} (z-component);x (mm); z (mm); field strength (1/cm)",
                              static_cast<int>(steps),
                              -1.5 * model->getPixelSize().x(),
                              1.5 * model->getPixelSize().x(),
                              static_cast<int>(steps),
                              min,
                              max);
    // histogram->SetMinimum(0);

    // Get the weighting field at every index
    for(size_t j = 0; j < steps; ++j) {
        LOG_PROGRESS(INFO, "plotting") << "Plotting progress " << 100 * j * steps / (steps * steps) << "%";
        for(size_t k = 0; k < steps; ++k) {
            double z = min + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * (max - min);
            double x = -0.5 * model->getPixelSize().x() +
                       ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * 3 * model->getPixelSize().x();

            // LOG(DEBUG) << "Requesting field ad local coords (" << Units::display(x, {"cm", "mm", "um"}) << "," << 0;
            // Get field strength from detector
            auto field = detector_->getWeightingField(ROOT::Math::XYZPoint(x, 0, z), 1, 0);
            auto field_z_strength = Units::convert(field.z(), "1/cm");

            // Fill the histograms, shift x-axis by one pixel so the reference electrode in centered.
            histogram->Fill(x - model->getPixelSize().x(), z, static_cast<double>(field_z_strength));
        }
    }

    // Write the histogram to module file
    histogramX->Write();
    histogramY->Write();
    histogramZ->Write();
    histogram->Write();
}

/**
 * The field read from the INIT format are shared between module instantiations
 * using the static WeightingFieldReaderModule::get_by_file_name method.
 */
WeightingFieldReaderModule::FieldData WeightingFieldReaderModule::read_init_field() {
    using namespace ROOT::Math;

    try {
        LOG(TRACE) << "Fetching weighting field from init file";

        // Get field from file
        auto field_data = get_by_file_name(config_.getPath("file_name", true), *detector_.get());
        LOG(INFO) << "Set weighting field with " << field_data.second.second.at(0) << "x" << field_data.second.second.at(1)
                  << "x" << field_data.second.second.at(2) << " cells";

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

/**
 * @brief Check if the detector matches the file header
 */
inline static void check_detector_match(Detector& detector, double thickness, double xpixsz, double ypixsz) {
    auto model = detector.getModel();
    // Do a several checks with the detector model
    if(model != nullptr) {
        if(std::fabs(thickness - model->getSensorSize().z()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Thickness of sensor in field map file is " << Units::display(thickness, "um")
                         << " but in the detector model it is " << Units::display(model->getSensorSize().z(), "um");
        }

        // Check that the total field size is n*pitch:
        if(std::fmod(xpixsz, model->getPixelSize().x()) > std::numeric_limits<double>::epsilon() ||
           std::fmod(ypixsz, model->getPixelSize().y()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Field size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"}) << ") but expecting a multiple of the pixel pitch ("
                         << Units::display(model->getPixelSize().x(), {"um", "mm"}) << ", "
                         << Units::display(model->getPixelSize().y(), {"um", "mm"}) << ")";
        }
    }
}

std::map<std::string, WeightingFieldReaderModule::FieldData> WeightingFieldReaderModule::field_map_;
WeightingFieldReaderModule::FieldData WeightingFieldReaderModule::get_by_file_name(const std::string& file_name,
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

    // Check if weighting field matches chip
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

        // Get index of weighting field
        size_t xind, yind, zind;
        file >> xind >> yind >> zind;

        if(file.fail() || xind > xsize || yind > ysize || zind > zsize) {
            throw std::runtime_error("invalid data");
        }
        xind--;
        yind--;
        zind--;

        // Loop through components of weighting field
        for(size_t j = 0; j < 3; ++j) {
            double input;
            file >> input;

            // Set the weighting field at a position
            (*field)[xind * ysize * zsize * 3 + yind * zsize * 3 + zind * 3 + j] = Units::get(input, "V/cm");
        }
    }

    return std::make_pair(
        field,
        std::make_pair(std::array<double, 3>{{xpixsz, ypixsz, thickness}}, std::array<size_t, 3>{{xsize, ysize, zsize}}));
}
