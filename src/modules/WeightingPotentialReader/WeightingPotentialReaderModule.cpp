/**
 * @file
 * @brief Implementation of module to read weighting fields
 * @copyright Copyright (c) 2019-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();
}

void WeightingPotentialReaderModule::initialize() {

    auto field_model = config_.get<std::string>("model");

    // Calculate thickness domain
    auto model = detector_->getModel();
    auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    auto thickness_domain = std::make_pair(sensor_max_z - model->getSensorSize().z(), sensor_max_z);

    // Calculate the potential depending on the configuration
    if(field_model == "mesh") {
        auto field_data = read_field(thickness_domain);

        detector_->setWeightingPotentialGrid(field_data.getData(),
                                             field_data.getDimensions(),
                                             std::array<double, 2>{{field_data.getSize()[0], field_data.getSize()[1]}},
                                             std::array<double, 2>{{0, 0}},
                                             thickness_domain);
    } else if(field_model == "pad") {
        LOG(TRACE) << "Adding weighting potential from pad in plane condenser";

        // Get pixel implant size from the detector model:
        auto implant = model->getImplantSize();
        auto function = get_pad_potential_function(implant, thickness_domain);
        detector_->setWeightingPotentialFunction(function, thickness_domain, FieldType::CUSTOM);
    } else {
        throw InvalidValueError(config_, "model", "model should be 'init' or `pad`");
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

    auto steps = config_.get<size_t>("output_plots_steps", 500);
    auto position = config_.get<ROOT::Math::XYPoint>("output_plots_position", ROOT::Math::XYPoint(0, 0));

    auto model = detector_->getModel();

    double min = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0;
    double max = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;

    // Create 1D histograms
    auto* histogram = new TH1F("potential1d", "#phi_{w}/V_{w};z (mm);unit potential", static_cast<int>(steps), min, max);

    // Get the weighting potential at every index
    for(size_t j = 0; j < steps; ++j) {
        double z = min + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * (max - min);
        auto pos = ROOT::Math::XYZPoint(position.x(), position.y(), z);

        // Get potential from detector and fill the histogram
        auto potential = detector_->getWeightingPotential(pos, Pixel::Index(0, 0));
        histogram->Fill(z, potential);
    }

    // Create 2D histogram
    auto* histogram2Dx = new TH2F("potential_x",
                                  "#phi_{w}/V_{w};x (mm); z (mm); unit potential",
                                  static_cast<int>(steps),
                                  -1.5 * model->getPixelSize().x(),
                                  1.5 * model->getPixelSize().x(),
                                  static_cast<int>(steps),
                                  min,
                                  max);

    // Create 2D histogram
    auto* histogram2Dy = new TH2F("potential_y",
                                  "#phi_{w}/V_{w};y (mm); z (mm); unit potential",
                                  static_cast<int>(steps),
                                  -1.5 * model->getPixelSize().y(),
                                  1.5 * model->getPixelSize().y(),
                                  static_cast<int>(steps),
                                  min,
                                  max);

    // Get the weighting potential at every index
    for(size_t j = 0; j < steps; ++j) {
        LOG_PROGRESS(INFO, "plotting") << "Plotting weighting potential: " << 100 * j * steps / (steps * steps) << "%";
        double z = min + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * (max - min);

        // Scan horizontally over three pixels (from -1.5 pitch to +1.5 pitch)
        for(size_t k = 0; k < steps; ++k) {
            double x = -0.5 * model->getPixelSize().x() +
                       ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * 3 * model->getPixelSize().x();
            double y = -0.5 * model->getPixelSize().y() +
                       ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * 3 * model->getPixelSize().y();

            // Get potential from detector and fill histogram. We calculate relative to pixel (1,0) so we need to shift x:
            auto potential_x = detector_->getWeightingPotential(ROOT::Math::XYZPoint(x, 0, z), Pixel::Index(1, 0));
            // Same holds along y axis:
            auto potential_y = detector_->getWeightingPotential(ROOT::Math::XYZPoint(0, y, z), Pixel::Index(0, 1));

            histogram2Dx->Fill(x - model->getPixelSize().x(), z, potential_x);
            histogram2Dy->Fill(y - model->getPixelSize().y(), z, potential_y);
        }
    }
    LOG_PROGRESS(INFO, "plotting") << "Plotting weighting potential: done ";

    histogram->SetOption("hist");
    histogram2Dx->SetOption("colz");
    histogram2Dy->SetOption("colz");

    // Write the histogram to module file
    histogram->Write();
    histogram2Dx->Write();
    histogram2Dy->Write();
}

/**
 * The field data read from files are shared between module instantiations
 * using the static FieldParser's getByFileName method.
 */
FieldParser<double> WeightingPotentialReaderModule::field_parser_(FieldQuantity::SCALAR);
FieldData<double> WeightingPotentialReaderModule::read_field(std::pair<double, double> thickness_domain) {
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

        // Check if weigthing potential matches chip
        check_detector_match(field_data.getSize(), thickness_domain);

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

/**
 * @brief Check if the detector matches the file header
 */
void WeightingPotentialReaderModule::check_detector_match(std::array<double, 3> dimensions,
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
            LOG(WARNING) << "Thickness of weighting potential is " << Units::display(thickness, "um")
                         << " but the depleted region is " << Units::display(eff_thickness, "um");
        }

        // Check that the total field size is n*pitch:
        if(std::fabs(std::remainder(xpixsz, model->getPixelSize().x())) > std::numeric_limits<double>::epsilon() ||
           std::fabs(std::remainder(ypixsz, model->getPixelSize().y())) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Potential map size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"}) << ") but expecting a multiple of the pixel pitch ("
                         << Units::display(model->getPixelSize().x(), {"um", "mm"}) << ", "
                         << Units::display(model->getPixelSize().y(), {"um", "mm"}) << ")";
        } else {
            LOG(INFO) << "Potential map size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                      << Units::display(ypixsz, {"um", "mm"}) << "), matching detector model with pixel pitch ("
                      << Units::display(model->getPixelSize().x(), {"um", "mm"}) << ", "
                      << Units::display(model->getPixelSize().y(), {"um", "mm"}) << ")";
        }
    }
}
