/**
 * @file
 * @brief Implementation of module to read electric fields
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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
#include <TFormula.h>
#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

ElectricFieldReaderModule::ElectricFieldReaderModule(Configuration& config, Messenger*, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Set default units for interpreting input field files in:
    config_.setDefault("file_units", "V/cm");

    // NOTE use voltage as a synonym for bias voltage
    config_.setAlias("bias_voltage", "voltage");
    // NOTE use field_depth as a synonym for depletion_depth
    config_.setAlias("depletion_depth", "field_depth");
}

void ElectricFieldReaderModule::initialize() {

    // Check field strength
    auto field_model = config_.get<ElectricField>("model");
    if((field_model == ElectricField::CONSTANT || field_model == ElectricField::LINEAR) &&
       config_.get<double>("bias_voltage") > Units::get(5.0, "kV")) {
        LOG(WARNING) << "Very high bias voltage of " << Units::display(config_.get<double>("bias_voltage"), "kV")
                     << " set, this is most likely not desired.";
    }

    // Check we don't have both depletion depth and depletion voltage:
    if(config_.count({"depletion_voltage", "depletion_depth"}) > 1) {
        throw InvalidCombinationError(
            config_, {"depletion_voltage", "depletion_depth"}, "Depletion voltage and depth are mutually exclusive.");
    }

    // Set depletion depth to full sensor:
    auto model = detector_->getModel();
    auto depletion_depth = config_.get<double>("depletion_depth", model->getSensorSize().z());
    if(depletion_depth - model->getSensorSize().z() > 1e-9) {
        throw InvalidValueError(config_, "depletion_depth", "depletion depth can not be larger than the sensor thickness");
    }

    // Calculate thickness domain
    auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    auto thickness_domain = std::make_pair(sensor_max_z - depletion_depth, sensor_max_z);

    // Calculate the field depending on the configuration
    if(field_model == ElectricField::MESH) {
        // Read field mapping from configuration
        auto field_mapping = config_.get<FieldMapping>("field_mapping");
        LOG(DEBUG) << "Electric field maps to " << magic_enum::enum_name(field_mapping);
        auto field_data = read_field();

        // By default, set field scale from physical extent read from field file:
        std::array<double, 2> field_scale{{1.0, 1.0}};
        // Read the field scales from the configuration if the key is set:
        if(config_.has("field_scale")) {
            auto scales = config_.get<ROOT::Math::XYVector>("field_scale", {1.0, 1.0});
            // FIXME Add sanity checks for scales here
            LOG(DEBUG) << "Electric field will be scaled with factors " << scales;
            field_scale = {{scales.x(), scales.y()}};
        }

        // Get the field offset in fractions of the field size, default is 0.0x0.0, i.e. no offset
        auto offset = config_.get<ROOT::Math::XYVector>("field_offset", {0.0, 0.0});
        if(offset.x() > 1.0 || offset.y() > 1.0) {
            throw InvalidValueError(
                config_, "field_offset", "shifting electric field by more than one pixel (offset > 1.0) is not allowed");
        }
        if(offset.x() < 0.0 || offset.y() < 0.0) {
            throw InvalidValueError(config_, "field_offset", "offsets for the electric field have to be positive");
        }
        LOG(DEBUG) << "Electric field has offset of " << offset << " fractions of the field size";

        detector_->setElectricFieldGrid(field_data.getData(),
                                        field_data.getDimensions(),
                                        field_data.getSize(),
                                        field_mapping,
                                        field_scale,
                                        {{offset.x(), offset.y()}},
                                        thickness_domain);
    } else if(field_model == ElectricField::CONSTANT) {
        LOG(TRACE) << "Adding constant electric field";
        auto field_z = config_.get<double>("bias_voltage") / getDetector()->getModel()->getSensorSize().z();
        LOG(INFO) << "Set constant electric field with magnitude " << Units::display(field_z, {"V/um", "V/mm"});
        FieldFunction<ROOT::Math::XYZVector> function = [field_z](const ROOT::Math::XYZPoint&) {
            return ROOT::Math::XYZVector(0, 0, field_z);
        };
        detector_->setElectricFieldFunction(std::move(function), thickness_domain, FieldType::CONSTANT);
    } else if(field_model == ElectricField::LINEAR) {
        LOG(TRACE) << "Adding linear electric field";

        // Get depletion voltage, defaults to bias voltage:
        auto depletion_voltage = config_.get<double>("depletion_voltage", config_.get<double>("bias_voltage"));

        LOG(INFO) << "Setting linear electric field from " << Units::display(config_.get<double>("bias_voltage"), "V")
                  << " bias voltage and " << Units::display(depletion_voltage, "V") << " depletion voltage";
        detector_->setElectricFieldFunction(
            get_linear_field_function(depletion_voltage, thickness_domain), thickness_domain, FieldType::LINEAR);
    } else if(field_model == ElectricField::PARABOLIC) {
        LOG(TRACE) << "Adding parabolic electric field";
        LOG(INFO) << "Setting parabolic electric field with minimum field "
                  << Units::display(config_.get<double>("minimum_field"), "V/cm") << " at position "
                  << Units::display(config_.get<double>("minimum_position"), {"um", "mm"}) << " and maximum field "
                  << Units::display(config_.get<double>("maximum_field"), "V/cm") << " at electrode";
        detector_->setElectricFieldFunction(
            get_parabolic_field_function(thickness_domain), thickness_domain, FieldType::CUSTOM1D);
    } else if(field_model == ElectricField::CUSTOM) {
        LOG(TRACE) << "Adding custom electric field";
        auto [field_function, field_type] = get_custom_field_function();
        detector_->setElectricFieldFunction(field_function, thickness_domain, field_type);
    }

    // Produce histograms if needed
    if(config_.get<bool>("output_plots", false)) {
        create_output_plots();
    }
}

FieldFunction<ROOT::Math::XYZVector>
ElectricFieldReaderModule::get_linear_field_function(double depletion_voltage, std::pair<double, double> thickness_domain) {
    LOG(TRACE) << "Calculating function for the linear electric field.";
    // We always deplete from the implants:
    auto bias_voltage = std::fabs(config_.get<double>("bias_voltage"));
    depletion_voltage = std::fabs(depletion_voltage);
    // But the direction of the field depends on the applied voltage:
    auto direction = std::signbit(config_.get<double>("bias_voltage"));
    auto deplete_from_implants = config_.get<bool>("deplete_from_implants", true);
    double eff_thickness = thickness_domain.second - thickness_domain.first;
    // Reduce the effective thickness of the sensor if voltage is below full depletion
    if(bias_voltage < depletion_voltage) {
        eff_thickness *= sqrt(bias_voltage / depletion_voltage);
        depletion_voltage = bias_voltage;
    }
    LOG(TRACE) << "Effective thickness of the electric field: " << Units::display(eff_thickness, {"um", "mm"});
    LOG(DEBUG) << "Depleting the sensor from the " << (deplete_from_implants ? "implant side." : "back side.");
    return [bias_voltage, depletion_voltage, direction, eff_thickness, thickness_domain, deplete_from_implants](
               const ROOT::Math::XYZPoint& pos) {
        double z_rel = thickness_domain.second - pos.z();
        double field_z = std::max(0.0,
                                  (bias_voltage - depletion_voltage) / eff_thickness +
                                      2 * (depletion_voltage / eff_thickness) *
                                          (deplete_from_implants ? (1 - z_rel / eff_thickness) : z_rel / eff_thickness));
        return ROOT::Math::XYZVector(0, 0, (direction ? -1 : 1) * field_z);
    };
}

FieldFunction<ROOT::Math::XYZVector>
ElectricFieldReaderModule::get_parabolic_field_function(std::pair<double, double> thickness_domain) {
    LOG(TRACE) << "Calculating function for the parabolic electric field.";

    auto z_min = config_.get<double>("minimum_position");
    auto e_max = config_.get<double>("maximum_field");
    double eff_thickness = thickness_domain.second - thickness_domain.first;

    if(z_min <= thickness_domain.first || z_min >= thickness_domain.second) {
        throw InvalidValueError(config_,
                                "minimum_position",
                                "Minimum field position must be within defined region of the electric field (" +
                                    Units::display(thickness_domain.first, "um") + "," +
                                    Units::display(thickness_domain.second, "um") + ")");
    }

    auto a = (e_max - config_.get<double>("minimum_field")) /
             (z_min * z_min + thickness_domain.second * thickness_domain.second - eff_thickness * z_min);
    auto b = -2 * a * z_min;
    auto c = e_max - a * (thickness_domain.second * thickness_domain.second - eff_thickness * z_min);

    return [a, b, c](const ROOT::Math::XYZPoint& pos) {
        double field_z = a * pos.z() * pos.z() + b * pos.z() + c;
        return ROOT::Math::XYZVector(0, 0, field_z);
    };
}

std::pair<FieldFunction<ROOT::Math::XYZVector>, FieldType> ElectricFieldReaderModule::get_custom_field_function() {

    auto field_functions = config_.getArray<std::string>("field_function");
    auto field_parameters = config_.getArray<double>("field_parameters");

    // 1D field, interpret as field along z-axis:
    if(field_functions.size() == 1) {
        LOG(DEBUG) << "Found definition of 1D custom field, applying to z axis";
        auto z = std::make_shared<TFormula>("ez", field_functions.front().c_str(), false);

        // Check if number of parameters match up
        if(static_cast<size_t>(z->GetNpar()) != field_parameters.size()) {
            throw InvalidValueError(
                config_,
                "field_parameters",
                "The number of function parameters does not line up with the amount of parameters in the function.");
        }

        // Apply parameters to the function
        for(size_t n = 0; n < field_parameters.size(); ++n) {
            z->SetParameter(static_cast<int>(n), field_parameters.at(n));
        }

        LOG(DEBUG) << "Value of custom field at pixel center: " << Units::display(z->Eval(0., 0., 0.), "V/cm");
        return {[z = std::move(z)](const ROOT::Math::XYZPoint& pos) {
                    return ROOT::Math::XYZVector(0, 0, z->Eval(pos.x(), pos.y(), pos.z()));
                },
                FieldType::CUSTOM1D};
    } else if(field_functions.size() == 3) {
        LOG(DEBUG) << "Found definition of 3D custom field, applying to three Cartesian axes";
        auto x = std::make_shared<TFormula>("ex", field_functions.at(0).c_str(), false);
        auto y = std::make_shared<TFormula>("ey", field_functions.at(1).c_str(), false);
        auto z = std::make_shared<TFormula>("ez", field_functions.at(2).c_str(), false);

        // Check if number of parameters match up
        if(static_cast<size_t>(x->GetNpar() + y->GetNpar() + z->GetNpar()) != field_parameters.size()) {
            throw InvalidValueError(
                config_,
                "field_parameters",
                "The number of function parameters does not line up with the sum of parameters in all functions.");
        }

        // Apply parameters to the functions
        for(auto n = 0; n < x->GetNpar(); ++n) {
            x->SetParameter(n, field_parameters.at(static_cast<size_t>(n)));
        }
        for(auto n = 0; n < y->GetNpar(); ++n) {
            y->SetParameter(n, field_parameters.at(static_cast<size_t>(n + x->GetNpar())));
        }
        for(auto n = 0; n < z->GetNpar(); ++n) {
            z->SetParameter(n, field_parameters.at(static_cast<size_t>(n + x->GetNpar() + y->GetNpar())));
        }

        LOG(DEBUG) << "Value of custom field at pixel center: "
                   << Units::display(ROOT::Math::XYZVector(x->Eval(0., 0., 0.), y->Eval(0., 0., 0.), z->Eval(0., 0., 0.)),
                                     {"V/cm"});
        return {[x = std::move(x), y = std::move(y), z = std::move(z)](const ROOT::Math::XYZPoint& pos) {
                    return ROOT::Math::XYZVector(x->Eval(pos.x(), pos.y(), pos.z()),
                                                 y->Eval(pos.x(), pos.y(), pos.z()),
                                                 z->Eval(pos.x(), pos.y(), pos.z()));
                },
                FieldType::CUSTOM};
    } else {
        throw InvalidValueError(config_,
                                "field_function",
                                "field function either needs one component (z) or three components (x,y,z) but " +
                                    std::to_string(field_functions.size()) + " were given");
    }
}

/**
 * The field data read from files are shared between module instantiations using the static
 * FieldParser's getByFileName method.
 */
FieldParser<double> ElectricFieldReaderModule::field_parser_(FieldQuantity::VECTOR);
FieldData<double> ElectricFieldReaderModule::read_field() {

    try {
        LOG(TRACE) << "Fetching electric field from mesh file";

        // Get field from file
        auto field_data = field_parser_.getByFileName(config_.getPath("file_name", true), config_.get<std::string>("file_units"));

        // Warn at field values larger than 1MV/cm / 10 MV/mm. Simple lookup per vector component, not total field magnitude
        auto max_field = *std::max_element(std::begin(*field_data.getData()), std::end(*field_data.getData()));
        if(max_field > 10) {
            LOG(WARNING) << "Very high electric field of " << Units::display(max_field, "kV/cm")
                         << ", this is most likely not desired.";
        }

        LOG(INFO) << "Set electric field with " << field_data.getDimensions().at(0) << "x"
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

void ElectricFieldReaderModule::create_output_plots() {
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

    // Create 2D histograms
    auto* histogram = new TH2F("field_magnitude",
                               "Electric field magnitude",
                               static_cast<int>(steps),
                               min1,
                               max1,
                               static_cast<int>(steps),
                               min2,
                               max2);
    histogram->SetMinimum(-0.01);
    histogram->SetOption("colz");

    auto* histogram_x = new TH2F(
        "field_x", "Electric field (x-component)", static_cast<int>(steps), min1, max1, static_cast<int>(steps), min2, max2);
    auto* histogram_y = new TH2F(
        "field_y", "Electric field (y-component)", static_cast<int>(steps), min1, max1, static_cast<int>(steps), min2, max2);
    auto* histogram_z = new TH2F(
        "field_z", "Electric field (z-component)", static_cast<int>(steps), min1, max1, static_cast<int>(steps), min2, max2);
    auto* histogram_lateral = new TH2F(
        "field_lateral", "Lateral electric field", static_cast<int>(steps), min1, max1, static_cast<int>(steps), min2, max2);
    histogram_x->SetOption("colz");
    histogram_y->SetOption("colz");
    histogram_z->SetOption("colz");
    histogram_lateral->SetOption("colz");

    // Create 1D histogram
    auto* histogram1D = new TH1F(
        "field1d_z", "Electric field (z-component);z (mm);field strength (V/cm)", static_cast<int>(steps), min2, max2);
    histogram1D->SetOption("hist");

    // Determine the coordinate to use for projection
    double x = 0, y = 0, z = 0;
    if(project == 'x') {
        x = center.x() - size.x() / 2.0 + config_.get<double>("output_plots_projection_percentage", 0.5) * size.x();
        histogram->GetXaxis()->SetTitle("y (mm)");
        histogram_x->GetXaxis()->SetTitle("y (mm)");
        histogram_y->GetXaxis()->SetTitle("y (mm)");
        histogram_z->GetXaxis()->SetTitle("y (mm)");
        histogram_lateral->GetXaxis()->SetTitle("y (mm)");
        histogram->GetYaxis()->SetTitle("z (mm)");
        histogram_x->GetYaxis()->SetTitle("z (mm)");
        histogram_y->GetYaxis()->SetTitle("z (mm)");
        histogram_z->GetYaxis()->SetTitle("z (mm)");
        histogram_lateral->GetYaxis()->SetTitle("z (mm)");
        histogram->SetTitle(("Electric field magnitude at x=" + std::to_string(x) + " mm").c_str());
        histogram_x->SetTitle(("Electric field (x-component) at x=" + std::to_string(x) + " mm").c_str());
        histogram_y->SetTitle(("Electric field (y-component) at x=" + std::to_string(x) + " mm").c_str());
        histogram_z->SetTitle(("Electric field (z-component) at x=" + std::to_string(x) + " mm").c_str());
        histogram_lateral->SetTitle(("Lateral electric field at x=" + std::to_string(x) + " mm").c_str());
    } else if(project == 'y') {
        y = center.y() - size.y() / 2.0 + config_.get<double>("output_plots_projection_percentage", 0.5) * size.y();
        histogram->GetXaxis()->SetTitle("x (mm)");
        histogram_x->GetXaxis()->SetTitle("x (mm)");
        histogram_y->GetXaxis()->SetTitle("x (mm)");
        histogram_z->GetXaxis()->SetTitle("x (mm)");
        histogram_lateral->GetXaxis()->SetTitle("x (mm)");
        histogram->GetYaxis()->SetTitle("z (mm)");
        histogram_x->GetYaxis()->SetTitle("z (mm)");
        histogram_y->GetYaxis()->SetTitle("z (mm)");
        histogram_z->GetYaxis()->SetTitle("z (mm)");
        histogram_lateral->GetYaxis()->SetTitle("z (mm)");
        histogram->SetTitle(("Electric field magnitude at y=" + std::to_string(y) + " mm").c_str());
        histogram_x->SetTitle(("Electric field (x-component) at y=" + std::to_string(y) + " mm").c_str());
        histogram_y->SetTitle(("Electric field (y-component) at y=" + std::to_string(y) + " mm").c_str());
        histogram_z->SetTitle(("Electric field (z-component) at y=" + std::to_string(y) + " mm").c_str());
        histogram_lateral->SetTitle(("Lateral electric field at y=" + std::to_string(y) + " mm").c_str());
    } else {
        z = z_min + config_.get<double>("output_plots_projection_percentage", 0.5) * size.z();
        histogram->GetXaxis()->SetTitle("x (mm)");
        histogram_x->GetXaxis()->SetTitle("x (mm)");
        histogram_y->GetXaxis()->SetTitle("x (mm)");
        histogram_z->GetXaxis()->SetTitle("x (mm)");
        histogram_lateral->GetXaxis()->SetTitle("x (mm)");
        histogram->GetYaxis()->SetTitle("y (mm)");
        histogram_x->GetYaxis()->SetTitle("y (mm)");
        histogram_y->GetYaxis()->SetTitle("y (mm)");
        histogram_z->GetYaxis()->SetTitle("y (mm)");
        histogram_lateral->GetYaxis()->SetTitle("y (mm)");
        histogram->SetTitle(("Electric field magnitude at z=" + std::to_string(z) + " mm").c_str());
        histogram_x->SetTitle(("Electric field (x-component) at z=" + std::to_string(z) + " mm").c_str());
        histogram_y->SetTitle(("Electric field (y-component) at z=" + std::to_string(z) + " mm").c_str());
        histogram_z->SetTitle(("Electric field (z-component) at z=" + std::to_string(z) + " mm").c_str());
        histogram_lateral->SetTitle(("Lateral electric field at z=" + std::to_string(z) + " mm").c_str());
    }

    // set z axis tile
    histogram->GetZaxis()->SetTitle("field strength (V/cm)");
    histogram_x->GetZaxis()->SetTitle("field (V/cm)");
    histogram_y->GetZaxis()->SetTitle("field (V/cm)");
    histogram_z->GetZaxis()->SetTitle("field (V/cm)");
    histogram_lateral->GetZaxis()->SetTitle("field (V/cm)");

    // Find the electric field at every index, scan axes in local coordinates!
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

            // Get field strength from detector - directly convert to double to fill root histograms
            auto field = detector_->getElectricField(ROOT::Math::XYZPoint(x, y, z));
            auto field_strength = static_cast<double>(Units::convert(std::sqrt(field.Mag2()), "V/cm"));
            auto field_x_strength = static_cast<double>(Units::convert(field.x(), "V/cm"));
            auto field_y_strength = static_cast<double>(Units::convert(field.y(), "V/cm"));
            auto field_z_strength = static_cast<double>(Units::convert(field.z(), "V/cm"));
            auto field_lateral_strength = sqrt(field_x_strength * field_x_strength + field_y_strength * field_y_strength);

            // Fill the main histogram
            if(project == 'x') {
                histogram->Fill(y, z, field_strength);
                histogram_x->Fill(y, z, field_x_strength);
                histogram_y->Fill(y, z, field_y_strength);
                histogram_z->Fill(y, z, field_z_strength);
                histogram_lateral->Fill(y, z, field_lateral_strength);
            } else if(project == 'y') {
                histogram->Fill(x, z, field_strength);
                histogram_x->Fill(x, z, field_x_strength);
                histogram_y->Fill(x, z, field_y_strength);
                histogram_z->Fill(x, z, field_z_strength);
                histogram_lateral->Fill(x, z, field_lateral_strength);
            } else {
                histogram->Fill(x, y, field_strength);
                histogram_x->Fill(x, y, field_x_strength);
                histogram_y->Fill(x, y, field_y_strength);
                histogram_z->Fill(x, y, field_z_strength);
                histogram_lateral->Fill(x, y, field_lateral_strength);
            }
            // Fill the 1d histogram
            if(j == steps / 2) {
                histogram1D->Fill(z, field_z_strength);
            }
        }
    }

    // Write the histograms to module file
    histogram->Write();
    histogram_x->Write();
    histogram_y->Write();
    histogram_z->Write();
    histogram_lateral->Write();
    histogram1D->Write();
}
