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
        auto field_data = read_init_field(thickness_domain);

        // Calculate field scale from field size and pixel pitch:
        auto pixel_size = model->getPixelSize();
        std::array<double, 2> field_scale{
            {std::get<2>(field_data)[0] / pixel_size.x(), std::get<2>(field_data)[1] / pixel_size.y()}};

        detector_->setWeightingFieldGrid(
            std::get<0>(field_data), std::get<1>(field_data), field_scale, std::array<double, 2>{{0, 0}}, thickness_domain);
    } else if(field_model == "pad") {
        LOG(TRACE) << "Adding weighting field from pad in plane condenser";

        // Get pixel implant size from the detector model:
        auto implant = model->getImplantSize();
        FieldFunction<ROOT::Math::XYZVector> function = get_pad_field_function(implant, thickness_domain);
        detector_->setWeightingFieldFunction(function, thickness_domain, FieldType::CUSTOM);
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
FieldFunction<ROOT::Math::XYZVector>
WeightingFieldReaderModule::get_pad_field_function(const ROOT::Math::XYVector& implant,
                                                   std::pair<double, double> thickness_domain) {

    LOG(TRACE) << "Calculating function for the plane condenser weighting field." << std::endl;

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
        auto field = detector_->getWeightingField(ROOT::Math::XYZPoint(position.x(), position.y(), z), Pixel::Index(0, 0));
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
            auto field = detector_->getWeightingField(ROOT::Math::XYZPoint(x, 0, z), Pixel::Index(1, 0));
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
FieldParser<double, 3> WeightingFieldReaderModule::field_parser_("");
FieldData<double> WeightingFieldReaderModule::read_init_field(std::pair<double, double> thickness_domain) {
    using namespace ROOT::Math;

    try {
        LOG(TRACE) << "Fetching weighting field from init file";

        // Get field from file
        auto field_data = field_parser_.get_by_file_name(config_.getPath("file_name", true));

        // Check if electric field matches chip
        check_detector_match(std::get<2>(field_data), thickness_domain);

        LOG(INFO) << "Set weighting field with " << std::get<1>(field_data).at(0) << "x" << std::get<1>(field_data).at(1)
                  << "x" << std::get<1>(field_data).at(2) << " cells";

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
void WeightingFieldReaderModule::check_detector_match(std::array<double, 3> dimensions,
                                                      std::pair<double, double> thickness_domain) {
    auto xpixsz = dimensions[0];
    auto ypixsz = dimensions[1];
    auto thickness = dimensions[2];

    auto model = detector_->getModel();
    // Do a several checks with the detector model
    if(model != nullptr) {
        // Check field dimension in z versus the requested thickness domain:
        auto eff_thickness = thickness_domain.second - thickness_domain.first;
        if(std::fabs(thickness - eff_thickness) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Thickness of weighting field is " << Units::display(thickness, "um")
                         << " but the depleted region is " << Units::display(eff_thickness, "um");
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
