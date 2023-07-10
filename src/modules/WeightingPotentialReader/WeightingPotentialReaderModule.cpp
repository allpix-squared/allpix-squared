/**
 * @file
 * @brief Implementation of module to read weighting fields
 *
 * @copyright Copyright (c) 2018-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "WeightingPotentialReaderModule.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

WeightingPotentialReaderModule::WeightingPotentialReaderModule(Configuration& config,
                                                               Messenger*,
                                                               std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
}

void WeightingPotentialReaderModule::initialize() {

    auto field_model = config_.get<WeightingPotential>("model");

    // Calculate thickness domain
    auto model = detector_->getModel();
    auto potential_depth = config_.get<double>("potential_depth", model->getSensorSize().z());
    if(potential_depth - model->getSensorSize().z() > std::numeric_limits<double>::epsilon()) {
        throw InvalidValueError(
            config_, "potential_depth", "Weighting potential depth can not be larger than the sensor thickness");
    }
    auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    auto thickness_domain = std::make_pair(sensor_max_z - potential_depth, sensor_max_z);

    // Calculate the potential depending on the configuration
    if(field_model == WeightingPotential::MESH) {
        // Read field mapping from configuration
        auto field_mapping = config_.get<FieldMapping>("field_mapping");

        // SENSOR style mapping does not work for Weighting potentials, we always need to center on an electrode:
        if(field_mapping == FieldMapping::SENSOR) {
            throw InvalidValueError(
                config_, "field_mapping", "the weighting potential needs to be centered around an electrode");
        }
        LOG(DEBUG) << "Weighting potential maps to " << magic_enum::enum_name(field_mapping);
        auto field_data = read_field();

        // By default, set field scale from physical extent read from field file:
        std::array<double, 2> field_scale{{1.0, 1.0}};
        // Read the field scales from the configuration if the key is set:
        if(config_.has("field_scale")) {
            auto scales = config_.get<ROOT::Math::XYVector>("field_scale", {1.0, 1.0});
            // FIXME Add sanity checks for scales here
            LOG(DEBUG) << "Weighting potential will be scaled with factors " << scales;
            field_scale = {{scales.x(), scales.y()}};
        }

        // Get the field offset in fractions of the field size, default is 0.0x0.0, i.e. no offset
        auto offset = config_.get<ROOT::Math::XYVector>("field_offset", {0.0, 0.0});
        if(offset.x() > 1.0 || offset.y() > 1.0) {
            throw InvalidValueError(config_,
                                    "field_offset",
                                    "shifting weighting potential by more than one pixel (offset > 1.0) is not allowed");
        }
        if(offset.x() < 0.0 || offset.y() < 0.0) {
            throw InvalidValueError(config_, "field_offset", "offsets for the weighting potential have to be positive");
        }
        LOG(DEBUG) << "Weighting potential has offset of " << offset << " fractions of the field size";

        // Set the field grid, provide scale factors as fraction of the pixel pitch for correct scaling:
        detector_->setWeightingPotentialGrid(field_data.getData(),
                                             field_data.getDimensions(),
                                             field_data.getSize(),
                                             field_mapping,
                                             field_scale,
                                             {{offset.x(), offset.y()}},
                                             thickness_domain);
    } else if(field_model == WeightingPotential::PAD) {
        LOG(TRACE) << "Adding weighting potential from pad in plane condenser";

        // Get pixel implant size from the detector model:
        auto implants = model->getImplants();
        if(implants.size() > 1) {
            throw ModuleError("Detector model contains more than one implant, not supported for pad potential");
        }

        auto implant = (implants.empty() ? ROOT::Math::XYZVector(model->getPixelSize().x(), model->getPixelSize().y(), 0)
                                         : implants.front().getSize());
        // This module currently only works with pad definition, i.e. 2D implant deinition:
        if(implant.z() > std::numeric_limits<double>::epsilon()) {
            throw InvalidValueError(
                config_, "model", "model 'pad' can only be used with 2D implants, but non-zero thickness found");
        }

        auto function = get_pad_potential_function({implant.x(), implant.y()}, thickness_domain);
        detector_->setWeightingPotentialFunction(function, thickness_domain, FieldType::CUSTOM);
    }

    // Produce histograms if needed
    if(config_.get<bool>("output_plots", false)) {
        create_output_plots();
    }
}

/**
 * Implement weighting potential from doi:10.1016/j.nima.2014.08.044 and return it as a lookup function.
 */
FieldFunction<double>
WeightingPotentialReaderModule::get_pad_potential_function(const ROOT::Math::XYVector& implant,
                                                           std::pair<double, double> thickness_domain) {

    LOG(TRACE) << "Calculating function for the plane condenser weighting potential." << std::endl;

    return [implant, thickness_domain](const ROOT::Math::XYZPoint& pos) {
        // Calculate values of the "f" function
        auto f = [implant](double x, double y, double u) {
            // Calculate arctan fractions
            auto arctan = [](double a, double b, double c) {
                return std::atan(a * b / c / std::sqrt(a * a + b * b + c * c));
            };

            // Shift the x and y coordinates by plus/minus half the implant size:
            double x1 = x - implant.x() / 2;
            double x2 = x + implant.x() / 2;
            double y1 = y - implant.y() / 2;
            double y2 = y + implant.y() / 2;

            // Calculate arctan sum and return
            return arctan(x1, y1, u) + arctan(x2, y2, u) - arctan(x1, y2, u) - arctan(x2, y1, u);
        };

        // Transform into coordinate system with sensor between d/2 < z < -d/2:
        auto d = thickness_domain.second - thickness_domain.first;
        auto local_z = -pos.z() + thickness_domain.second;

        // Calculate the series expansion
        double sum = 0;
        for(int n = 1; n <= 100; n++) {
            sum += f(pos.x(), pos.y(), 2 * n * d - local_z) - f(pos.x(), pos.y(), 2 * n * d + local_z);
        }

        return (1 / (2 * M_PI) * (f(pos.x(), pos.y(), local_z) - sum));
    };
}

void WeightingPotentialReaderModule::create_output_plots() {
    LOG(TRACE) << "Creating output plots";

    auto model = detector_->getModel();

    auto center = model->getPixelCenter(1, 1);
    auto size =
        ROOT::Math::XYZVector(3 * model->getPixelSize().x(), 3 * model->getPixelSize().y(), model->getSensorSize().z());

    auto position = config_.get<ROOT::Math::XYPoint>("output_plots_position", {center.x(), center.y()});
    auto steps = config_.get<size_t>("output_plots_steps", 500);

    double x_min = center.x() - size.x() / 2.0;
    double x_max = center.x() + size.x() / 2.0;
    double y_min = center.y() - size.y() / 2.0;
    double y_max = center.y() + size.y() / 2.0;
    double z_min = center.z() - size.z() / 2.0;
    double z_max = center.z() + size.z() / 2.0;

    // Create 1D histograms
    std::string title = "#phi_{w}/V_{w} at " + Units::display(position, {"um"}) + ";z (mm);unit potential";
    auto* histogram = new TH1F("potential1d", title.c_str(), static_cast<int>(steps), z_min, z_max);

    // Get the weighting potential at every index
    for(size_t j = 0; j < steps; ++j) {
        double z = z_min + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * (z_max - z_min);
        auto pos = ROOT::Math::XYZPoint(position.x(), position.y(), z);

        // Get potential from detector and fill the histogram
        auto potential = detector_->getWeightingPotential(pos, Pixel::Index(1, 1));
        histogram->Fill(z, potential);
    }

    auto zcut = config_.get<double>("output_plots_zcut", 0.0);
    if(!model->isWithinSensor(ROOT::Math::XYZPoint(0, 0, zcut))) {
        throw InvalidValueError(config_, "output_plots_zcut", "Position is outside the sensor");
    }

    // Create 2D histograms
    auto* histogram2Dx = new TH2F("potential_x",
                                  "#phi_{w}/V_{w} of Pixel(1,1);x (mm); z (mm); unit potential",
                                  static_cast<int>(steps),
                                  x_min,
                                  x_max,
                                  static_cast<int>(steps),
                                  z_min,
                                  z_max);

    auto* histogram2Dy = new TH2F("potential_y",
                                  "#phi_{w}/V_{w} of Pixel(1,1);y (mm); z (mm); unit potential",
                                  static_cast<int>(steps),
                                  y_min,
                                  y_max,
                                  static_cast<int>(steps),
                                  z_min,
                                  z_max);

    auto* histogram2Dz = new TH2F("potential_z",
                                  "#phi_{w}/V_{w} of Pixel(1,1);x (mm); y (mm); unit potential",
                                  static_cast<int>(steps),
                                  x_min,
                                  x_max,
                                  static_cast<int>(steps),
                                  y_min,
                                  y_max);

    // Get the weighting potential at every index
    for(size_t j = 0; j < steps; ++j) {
        LOG_PROGRESS(INFO, "plotting") << "Plotting weighting potential: " << 100 * j * steps / (steps * steps) << "%";
        double z = z_min + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * (z_max - z_min);

        // Scan horizontally over three pixels (from -1.5 pitch to +1.5 pitch)
        for(size_t k = 0; k < steps; ++k) {
            double x =
                center.x() - size.x() / 2.0 + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * size.x();
            double y =
                center.y() - size.y() / 2.0 + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * size.y();

            // Get potential from detector and fill histogram. We calculate relative to pixel (1,1) so we need to shift:
            auto potential_x = detector_->getWeightingPotential(ROOT::Math::XYZPoint(x, center.y(), z), Pixel::Index(1, 1));
            auto potential_y = detector_->getWeightingPotential(ROOT::Math::XYZPoint(center.x(), y, z), Pixel::Index(1, 1));

            histogram2Dx->Fill(x, z, potential_x);
            histogram2Dy->Fill(y, z, potential_y);
        }
    }

    for(size_t j = 0; j < steps; ++j) {
        LOG_PROGRESS(INFO, "plotting") << "Plotting weighting potential: " << 100 * j * steps / (steps * steps) << "%";
        double x = center.x() - size.x() / 2.0 + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * size.x();
        // Scan horizontally over three pixels (from -1.5 pitch to +1.5 pitch)
        for(size_t k = 0; k < steps; ++k) {
            double y =
                center.y() - size.y() / 2.0 + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * size.y();
            auto potential_z = detector_->getWeightingPotential(ROOT::Math::XYZPoint(x, y, zcut), Pixel::Index(1, 1));
            histogram2Dz->Fill(x, y, potential_z);
        }
    }
    LOG_PROGRESS(INFO, "plotting") << "Plotting weighting potential: done ";

    histogram->SetOption("hist");
    histogram2Dx->SetOption("colz");
    histogram2Dy->SetOption("colz");
    histogram2Dz->SetOption("colz");

    // Write the histogram to module file
    histogram->Write();
    histogram2Dx->Write();
    histogram2Dy->Write();
    histogram2Dz->Write();
}

/**
 * The field data read from files are shared between module instantiations
 * using the static FieldParser's getByFileName method.
 */
FieldParser<double> WeightingPotentialReaderModule::field_parser_(FieldQuantity::SCALAR);
FieldData<double> WeightingPotentialReaderModule::read_field() {
    using namespace ROOT::Math;

    try {
        LOG(TRACE) << "Fetching weighting potential from init file";

        // Get field from file
        auto field_data = field_parser_.getByFileName(config_.getPath("file_name", true));

        // Check maximum/minimum values of the potential:
        auto elements = std::minmax_element(field_data.getData()->begin(), field_data.getData()->end());
        if(*elements.first < 0 || *elements.second > 1) {
            throw InvalidValueError(config_,
                                    "file_name",
                                    "Unphysical weighting potential detected, found " + std::to_string(*elements.first) +
                                        " < phi < " + std::to_string(*elements.second) + ", expected 0 < phi < 1");
        }

        // Check that we actually have a three-dimensional potential field, otherwise we get very unphysical results in
        // neighboring pixels along the "missing" dimension:
        if(field_data.getDimensionality() < 3) {
            // check if wrong dimensionality should be ignored
            if(config_.get<bool>("ignore_field_dimensions", false)) {
                LOG(WARNING) << "Weighting potential with " << std::to_string(field_data.getDimensionality())
                             << " dimensions detected, requiring three-dimensional scalar field - this might lead to "
                                "unexpected behavior.";
            } else {
                throw InvalidValueError(config_,
                                        "file_name",
                                        "Weighting potential with " + std::to_string(field_data.getDimensionality()) +
                                            " dimensions detected, requiring three-dimensional scalar field - this might "
                                            "lead to unexpected behavior.");
            }
        }

        LOG(INFO) << "Set weighting field with " << field_data.getDimensions()[0] << "x" << field_data.getDimensions()[1]
                  << "x" << field_data.getDimensions()[2] << " cells";

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
