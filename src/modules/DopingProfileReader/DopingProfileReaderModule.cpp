/**
 * @file
 * @brief Implementation of module to read doping concentration maps
 *
 * @copyright Copyright (c) 2018-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DopingProfileReaderModule.hpp"

#include <string>
#include <utility>

#include <TH2F.h>

#include "core/utils/log.h"

using namespace allpix;

DopingProfileReaderModule::DopingProfileReaderModule(Configuration& config, Messenger*, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
}

void DopingProfileReaderModule::initialize() {

    // Check field strength
    auto field_model = config_.get<DopingProfile>("model");

    // set depth of doping profile
    auto model = detector_->getModel();
    auto doping_depth = config_.get<double>("doping_depth", model->getSensorSize().z());
    if(doping_depth - model->getSensorSize().z() > std::numeric_limits<double>::epsilon()) {
        throw InvalidValueError(config_, "doping_depth", "doping depth can not be larger than the sensor thickness");
    }
    auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    auto thickness_domain = std::make_pair(sensor_max_z - doping_depth, sensor_max_z);

    // Calculate the field depending on the configuration
    if(field_model == DopingProfile::MESH) {
        // Read field mapping from configuration
        auto field_mapping = config_.get<FieldMapping>("field_mapping");
        LOG(DEBUG) << "Doping concentration maps to " << magic_enum::enum_name(field_mapping);

        auto field_data = read_field();

        // By default, set field scale from physical extent read from field file:
        std::array<double, 2> field_scale{{1.0, 1.0}};
        // Read the field scales from the configuration if the key is set:
        if(config_.has("field_scale")) {
            auto scales = config_.get<ROOT::Math::XYVector>("field_scale", {1.0, 1.0});
            // FIXME Add sanity checks for scales here
            LOG(DEBUG) << "Doping profile will be scaled with factors " << scales;
            field_scale = {{scales.x(), scales.y()}};
        }

        // Get the field offset in fractions of the field size, default is 0.0x0.0, i.e. no offset
        auto offset = config_.get<ROOT::Math::XYVector>("field_offset", {0.0, 0.0});
        if(offset.x() > 1.0 || offset.y() > 1.0) {
            throw InvalidValueError(
                config_, "field_offset", "shifting doping profile by more than one pixel (offset > 1.0) is not allowed");
        }
        if(offset.x() < 0.0 || offset.y() < 0.0) {
            throw InvalidValueError(config_, "field_offset", "offsets for the doping profile have to be positive");
        }
        LOG(DEBUG) << "Doping profile has offset of " << offset << " fractions of the field size";

        detector_->setDopingProfileGrid(field_data.getData(),
                                        field_data.getDimensions(),
                                        field_data.getSize(),
                                        field_mapping,
                                        field_scale,
                                        {{offset.x(), offset.y()}},
                                        thickness_domain);

    } else if(field_model == DopingProfile::CONSTANT) {
        LOG(TRACE) << "Adding constant doping concentration";

        auto concentration = config_.get<double>("doping_concentration");
        LOG(INFO) << "Set constant doping concentration of " << Units::display(concentration, {"/cm/cm/cm"});
        FieldFunction<double> function = [concentration](const ROOT::Math::XYZPoint&) noexcept { return concentration; };

        detector_->setDopingProfileFunction(std::move(function), FieldType::CONSTANT);
    } else if(field_model == DopingProfile::REGIONS) {
        LOG(TRACE) << "Adding doping concentration depending on sensor region";

        auto concentration = config_.getMatrix<double>("doping_concentration");
        std::map<double, double> concentration_map;
        for(const auto& region : concentration) {
            if(region.size() != 2) {
                throw InvalidValueError(
                    config_, "doping_concentration", "expecting two values per row, depth and concentration");
            }

            concentration_map[region.front()] = region.back();
            LOG(INFO) << "Set constant doping concentration of " << Units::display(region.back(), {"/cm/cm/cm"})
                      << " at sensor depth " << Units::display(region.front(), {"um", "mm"});
        }
        FieldFunction<double> function = [concentration_map, thickness = detector_->getModel()->getSensorSize().z()](
                                             const ROOT::Math::XYZPoint& position) {
            // Lower bound returns the first element that is *not less* than the given key - in this case, the z position
            // should always be *before* the region boundary set in the vector
            auto item = concentration_map.lower_bound(thickness / 2 - position.z());
            if(item != concentration_map.end()) {
                return item->second;
            } else {
                return concentration_map.rbegin()->second;
            }
        };

        detector_->setDopingProfileFunction(std::move(function), FieldType::CUSTOM1D);
    }

    // Produce doping_concentration_histograms if needed
    if(config_.get<bool>("output_plots", false)) {
        create_output_plots();
    }
}

/**
 * The field read from the INIT format are shared between module instantiations using the static FieldParser.
 */
FieldParser<double> DopingProfileReaderModule::field_parser_(FieldQuantity::SCALAR);
FieldData<double> DopingProfileReaderModule::read_field() {

    try {
        LOG(TRACE) << "Fetching doping concentration map from mesh file";

        // Get field from file
        auto field_data = field_parser_.getByFileName(config_.getPath("file_name", true), "/cm/cm/cm");

        LOG(INFO) << "Set doping concentration map with " << field_data.getDimensions().at(0) << "x"
                  << field_data.getDimensions().at(1) << "x" << field_data.getDimensions().at(2) << " cells";

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

void DopingProfileReaderModule::create_output_plots() {
    LOG(TRACE) << "Creating output plots";

    auto steps = config_.get<size_t>("output_plots_steps", 500);
    auto project = config_.get<char>("output_plots_project", 'x');

    if(project != 'x' && project != 'y' && project != 'z') {
        throw InvalidValueError(config_, "output_plots_project", "can only project on x, y or z axis");
    }

    auto model = detector_->getModel();

    // If we need to plot a single pixel, we use size and position of the pixel at the origin
    auto single_pixel = config_.get<bool>("output_plots_single_pixel", true);
    auto center = (single_pixel ? model->getPixelCenter(0, 0) : model->getSensorCenter());
    auto size =
        (single_pixel
             ? ROOT::Math::XYZVector(model->getPixelSize().x(), model->getPixelSize().y(), model->getSensorSize().z())
             : model->getSensorSize());

    double z_min = center.z() - size.z() / 2.0;
    double z_max = center.z() + size.z() / 2.0;

    // Determine minimum and maximum index depending on projection axis
    double min1 = NAN, max1 = NAN;
    double min2 = NAN, max2 = NAN;
    if(project == 'x') {
        min1 = center.y() - size.y() / 2.0;
        max1 = center.y() + size.y() / 2.0;
        min2 = z_min;
        max2 = z_max;
    } else if(project == 'y') {
        min1 = center.x() - size.x() / 2.0;
        max1 = center.x() + size.x() / 2.0;
        min2 = z_min;
        max2 = z_max;
    } else {
        min1 = center.x() - size.x() / 2.0;
        max1 = center.x() + size.x() / 2.0;
        min2 = center.y() - size.y() / 2.0;
        max2 = center.y() + size.y() / 2.0;
    }

    // Create 2D doping_concentration_histograms
    auto* doping_concentration_histogram = new TH2F("doping_concentration",
                                                    "Doping concentration (1/cm^{3})",
                                                    static_cast<int>(steps),
                                                    min1,
                                                    max1,
                                                    static_cast<int>(steps),
                                                    min2,
                                                    max2);
    doping_concentration_histogram->SetOption("colz");

    // Create 1D doping_concentration_histogram
    auto* doping_concentration_histogram1D = new TH1F("concentration1D_z",
                                                      "Doping concentration along z;z (mm);Doping concentration (1/cm^{3})",
                                                      static_cast<int>(steps),
                                                      min2,
                                                      max2);
    doping_concentration_histogram1D->SetOption("hist");

    // Determine the coordinate to use for projection
    double x = 0, y = 0, z = 0;
    if(project == 'x') {
        x = center.x() - size.x() / 2.0 + config_.get<double>("output_plots_projection_percentage", 0.5000001) * size.x();
        doping_concentration_histogram->GetXaxis()->SetTitle("y (mm)");
        doping_concentration_histogram->GetYaxis()->SetTitle("z (mm)");
        doping_concentration_histogram->SetTitle(
            ("Doping concentration (1/cm^{3}) at x=" + std::to_string(x) + " mm").c_str());
    } else if(project == 'y') {
        y = center.y() - size.y() / 2.0 + config_.get<double>("output_plots_projection_percentage", 0.5000001) * size.y();
        doping_concentration_histogram->GetXaxis()->SetTitle("x (mm)");
        doping_concentration_histogram->GetYaxis()->SetTitle("z (mm)");
        doping_concentration_histogram->SetTitle(
            ("Doping concentration (1/cm^{3}) at y=" + std::to_string(y) + " mm").c_str());
    } else {
        z = z_min + config_.get<double>("output_plots_projection_percentage", 0.5000001) * size.z();
        doping_concentration_histogram->GetXaxis()->SetTitle("x (mm)");
        doping_concentration_histogram->GetYaxis()->SetTitle("y (mm)");
        doping_concentration_histogram->SetTitle(
            ("Doping concentration (1/cm^{3}) at z=" + std::to_string(z) + " mm").c_str());
    }

    // set z axis tile
    doping_concentration_histogram->GetZaxis()->SetTitle("Concentration");

    // Find the doping concentration at every index, scan axes in local coordinates!
    for(size_t j = 0; j < steps; ++j) {
        if(project == 'x') {
            y = center.y() - size.y() / 2.0 + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * size.y();
        } else if(project == 'y') {
            x = center.x() - size.x() / 2.0 + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * size.x();
        } else {
            x = center.x() - size.x() / 2.0 + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * size.x();
        }
        for(size_t k = 0; k < steps; ++k) {
            if(project == 'x') {
                z = z_min + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * size.z();
            } else if(project == 'y') {
                z = z_min + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * size.z();
            } else {
                y = center.y() - size.y() / 2.0 + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * size.y();
            }

            // Get doping concentration from detector - directly convert to double
            // and 1/cm3 to fill root doping_concentration_histograms
            auto concentration = static_cast<double>(
                Units::convert(detector_->getDopingConcentration(ROOT::Math::XYZPoint(x, y, z)), "/cm/cm/cm"));
            // Fill the main doping_concentration_histogram
            if(project == 'x') {
                doping_concentration_histogram->Fill(y, z, concentration);
            } else if(project == 'y') {
                doping_concentration_histogram->Fill(x, z, concentration);
            } else {
                doping_concentration_histogram->Fill(x, y, concentration);
            }
            // Fill the 1d doping_concentration_histogram, in the middle of the range
            if(j == steps / 2) {
                doping_concentration_histogram1D->Fill(z, concentration);
            }
        }
    }

    // Write the doping_concentration_histograms to module file
    doping_concentration_histogram->Write();
    doping_concentration_histogram1D->Write();

    LOG(DEBUG) << "Maximum doping concentration within plotted cut: " << doping_concentration_histogram->GetMaximum()
               << " 1/cm3";
}
