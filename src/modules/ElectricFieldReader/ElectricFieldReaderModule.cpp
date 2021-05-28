/**
 * @file
 * @brief Implementation of module to read electric fields
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
#include <TF3.h>
#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

ElectricFieldReaderModule::ElectricFieldReaderModule(Configuration& config, Messenger*, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();

    // NOTE use voltage as a synonym for bias voltage
    config_.setAlias("bias_voltage", "voltage");
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
        // Read the field scales from the configuration, defaulting to 1.0x1.0 pixel cell:
        auto scales = config_.get<ROOT::Math::XYVector>("field_scale", {1.0, 1.0});
        // FIXME Add sanity checks for scales here
        LOG(DEBUG) << "Electric field will be scaled with factors " << scales;
        std::array<double, 2> field_scale{{model->getPixelSize().x() * scales.x(), model->getPixelSize().y() * scales.y()}};

        // Get the field offset in fractions of the pixel pitch, default is 0.0x0.0, i.e. starting at pixel boundary:
        auto offset = config_.get<ROOT::Math::XYVector>("field_offset", {0.0, 0.0});
        if(offset.x() > 1.0 || offset.y() > 1.0) {
            throw InvalidValueError(
                config_, "field_offset", "shifting electric field by more than one pixel (offset > 1.0) is not allowed");
        }
        if(offset.x() < 0.0 || offset.y() < 0.0) {
            throw InvalidValueError(config_, "field_offset", "offsets for the electric field have to be positive");
        }
        LOG(DEBUG) << "Electric field starts with offset " << offset << " to pixel boundary";
        std::array<double, 2> field_offset{{model->getPixelSize().x() * offset.x(), model->getPixelSize().y() * offset.y()}};

        auto field_data = read_field(thickness_domain, field_scale);

        detector_->setElectricFieldGrid(
            field_data.getData(), field_data.getDimensions(), field_scale, field_offset, thickness_domain);
    } else if(field_model == ElectricField::CONSTANT) {
        LOG(TRACE) << "Adding constant electric field";
        auto field_z = config_.get<double>("bias_voltage") / getDetector()->getModel()->getSensorSize().z();
        LOG(INFO) << "Set constant electric field with magnitude " << Units::display(field_z, {"V/um", "V/mm"});
        FieldFunction<ROOT::Math::XYZVector> function = [field_z](const ROOT::Math::XYZPoint&) {
            return ROOT::Math::XYZVector(0, 0, -field_z);
        };
        detector_->setElectricFieldFunction(function, thickness_domain, FieldType::CONSTANT);
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
            get_parabolic_field_function(thickness_domain), thickness_domain, FieldType::CUSTOM);
    } else if(field_model == ElectricField::CUSTOM) {
        LOG(TRACE) << "Adding custom electric field";
        detector_->setElectricFieldFunction(
            get_custom_field_function(thickness_domain), thickness_domain, FieldType::CUSTOM);
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

FieldFunction<ROOT::Math::XYZVector>
ElectricFieldReaderModule::get_custom_field_function(std::pair<double, double> thickness_domain) {

    auto field_functions = config_.getArray<std::string>("field_function");
    auto field_parameters = config_.getArray<double>("field_parameters");

    // Derive field extent from pixel boundaries:
    auto model = detector_->getModel();
    auto min = -0.5 * model->getPixelSize();
    auto max = 0.5 * model->getPixelSize();

    // 1D field, interpret as field along z-axis:
    if(field_functions.size() == 1) {
        LOG(DEBUG) << "Found definition of 1D custom field, applying to z axis";
        auto z = std::make_shared<TF3>("z",
                                       field_functions.front().c_str(),
                                       min.x(),
                                       max.x(),
                                       min.y(),
                                       max.y(),
                                       thickness_domain.first,
                                       thickness_domain.second);

        // Check if number of parameters match up
        if(static_cast<size_t>(z->GetNumberFreeParameters()) != field_parameters.size()) {
            throw InvalidValueError(
                config_,
                "field_parameters",
                "The number of function parameters does not line up with the amount of parameters in the function.");
        }

        // Apply parameters to the function
        for(size_t n = 0; n < field_parameters.size(); ++n) {
            z->SetParameter(static_cast<int>(n), field_parameters.at(n));
        }

        LOG(DEBUG) << "Value of custom field at sensor center: " << Units::display(z->Eval(0., 0., 0.), "V/cm");
        return [z = std::move(z)](const ROOT::Math::XYZPoint& pos) {
            return ROOT::Math::XYZVector(0, 0, z->Eval(pos.x(), pos.y(), pos.z()));
        };
    } else if(field_functions.size() == 3) {
        LOG(DEBUG) << "Found definition of 3D custom field, applying to three Cartesian axes";
        auto x = std::make_shared<TF3>("x",
                                       field_functions.at(0).c_str(),
                                       min.x(),
                                       max.x(),
                                       min.y(),
                                       max.y(),
                                       thickness_domain.first,
                                       thickness_domain.second);
        auto y = std::make_shared<TF3>("y",
                                       field_functions.at(1).c_str(),
                                       min.x(),
                                       max.x(),
                                       min.y(),
                                       max.y(),
                                       thickness_domain.first,
                                       thickness_domain.second);
        auto z = std::make_shared<TF3>("z",
                                       field_functions.at(2).c_str(),
                                       min.x(),
                                       max.x(),
                                       min.y(),
                                       max.y(),
                                       thickness_domain.first,
                                       thickness_domain.second);

        // Check if number of parameters match up
        if(static_cast<size_t>(x->GetNumberFreeParameters() + y->GetNumberFreeParameters() + z->GetNumberFreeParameters()) !=
           field_parameters.size()) {
            throw InvalidValueError(
                config_,
                "field_parameters",
                "The number of function parameters does not line up with the sum of parameters in all functions.");
        }

        // Apply parameters to the functions
        for(auto n = 0; n < x->GetNumberFreeParameters(); ++n) {
            x->SetParameter(n, field_parameters.at(static_cast<size_t>(n)));
        }
        for(auto n = 0; n < y->GetNumberFreeParameters(); ++n) {
            y->SetParameter(n, field_parameters.at(static_cast<size_t>(n + x->GetNumberFreeParameters())));
        }
        for(auto n = 0; n < z->GetNumberFreeParameters(); ++n) {
            z->SetParameter(
                n,
                field_parameters.at(static_cast<size_t>(n + x->GetNumberFreeParameters() + y->GetNumberFreeParameters())));
        }

        LOG(DEBUG) << "Value of custom field at sensor center: "
                   << Units::display(ROOT::Math::XYZVector(x->Eval(0., 0., 0.), y->Eval(0., 0., 0.), z->Eval(0., 0., 0.)),
                                     {"V/cm"});
        return [x = std::move(x), y = std::move(y), z = std::move(z)](const ROOT::Math::XYZPoint& pos) {
            return ROOT::Math::XYZVector(
                x->Eval(pos.x(), pos.y(), pos.z()), y->Eval(pos.x(), pos.y(), pos.z()), z->Eval(pos.x(), pos.y(), pos.z()));
        };
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
FieldData<double> ElectricFieldReaderModule::read_field(std::pair<double, double> thickness_domain,
                                                        std::array<double, 2> field_scale) {

    try {
        LOG(TRACE) << "Fetching electric field from mesh file";

        // Get field from file
        auto field_data = field_parser_.getByFileName(config_.getPath("file_name", true), "V/cm");

        // Check if electric field matches chip
        check_detector_match(field_data.getSize(), thickness_domain, field_scale);

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
    if(config_.get<bool>("output_plots_single_pixel", true)) {
        // If we need to plot a single pixel we change the model to fake the sensor to be a single pixel
        // NOTE: This is a little hacky, but is the easiest approach
        model = std::make_shared<DetectorModel>(*model);
        model->setSensorExcessTop(0);
        model->setSensorExcessBottom(0);
        model->setSensorExcessLeft(0);
        model->setSensorExcessRight(0);
        model->setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<unsigned int>>(1, 1));
    }
    // Use either full sensor axis or only depleted region
    double z_min = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0;
    double z_max = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;

    // Determine minimum and maximum index depending on projection axis
    double min1 = NAN, max1 = NAN;
    double min2 = NAN, max2 = NAN;
    if(project == 'x') {
        min1 = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0;
        max1 = model->getSensorCenter().y() + model->getSensorSize().y() / 2.0;
        min2 = z_min;
        max2 = z_max;
    } else if(project == 'y') {
        min1 = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0;
        max1 = model->getSensorCenter().x() + model->getSensorSize().x() / 2.0;
        min2 = z_min;
        max2 = z_max;
    } else {
        min1 = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0;
        max1 = model->getSensorCenter().x() + model->getSensorSize().x() / 2.0;
        min2 = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0;
        max2 = model->getSensorCenter().y() + model->getSensorSize().y() / 2.0;
    }

    // Create 2D histograms
    auto* histogram = new TH2F("field_magnitude",
                               "electric field magnitude",
                               static_cast<int>(steps),
                               min1,
                               max1,
                               static_cast<int>(steps),
                               min2,
                               max2);
    histogram->SetMinimum(0);
    histogram->SetOption("colz");

    auto* histogram_x = new TH2F(
        "field_x", "electric field (x-component)", static_cast<int>(steps), min1, max1, static_cast<int>(steps), min2, max2);
    auto* histogram_y = new TH2F(
        "field_y", "electric field (y-component)", static_cast<int>(steps), min1, max1, static_cast<int>(steps), min2, max2);
    auto* histogram_z = new TH2F(
        "field_z", "electric field (z-component)", static_cast<int>(steps), min1, max1, static_cast<int>(steps), min2, max2);
    histogram_x->SetOption("colz");
    histogram_y->SetOption("colz");
    histogram_z->SetOption("colz");

    // Create 1D histogram
    auto* histogram1D = new TH1F(
        "field1d_z", "electric field (z-component);z (mm);field strength (V/cm)", static_cast<int>(steps), min2, max2);

    // Determine the coordinate to use for projection
    double x = 0, y = 0, z = 0;
    if(project == 'x') {
        x = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0 +
            config_.get<double>("output_plots_projection_percentage", 0.5) * model->getSensorSize().x();
    } else if(project == 'y') {
        y = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0 +
            config_.get<double>("output_plots_projection_percentage", 0.5) * model->getSensorSize().y();
    } else {
        z = z_min + config_.get<double>("output_plots_projection_percentage", 0.5) * (z_max - z_min);
    }

    // set z axis tile
    histogram->GetZaxis()->SetTitle("field strength (V/cm)");
    histogram_x->GetZaxis()->SetTitle("field (V/cm)");
    histogram_y->GetZaxis()->SetTitle("field (V/cm)");
    histogram_z->GetZaxis()->SetTitle("field (V/cm)");
    // Find the electric field at every index
    for(size_t j = 0; j < steps; ++j) {
        if(project == 'x') {
            y = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().y();
            histogram->GetXaxis()->SetTitle("y (mm)");
            histogram_x->GetXaxis()->SetTitle("y (mm)");
            histogram_y->GetXaxis()->SetTitle("y (mm)");
            histogram_z->GetXaxis()->SetTitle("y (mm)");
        } else if(project == 'y') {
            x = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().x();
            histogram->GetXaxis()->SetTitle("x (mm)");
            histogram_x->GetXaxis()->SetTitle("x (mm)");
            histogram_y->GetXaxis()->SetTitle("x (mm)");
            histogram_z->GetXaxis()->SetTitle("x (mm)");
        } else {
            x = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().x();
            histogram->GetXaxis()->SetTitle("x (mm)");
            histogram_x->GetXaxis()->SetTitle("x (mm)");
            histogram_y->GetXaxis()->SetTitle("x (mm)");
            histogram_z->GetXaxis()->SetTitle("x (mm)");
        }
        for(size_t k = 0; k < steps; ++k) {
            if(project == 'x') {
                z = z_min + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * (z_max - z_min);
                histogram->GetYaxis()->SetTitle("z (mm)");
                histogram_x->GetYaxis()->SetTitle("z (mm)");
                histogram_y->GetYaxis()->SetTitle("z (mm)");
                histogram_z->GetYaxis()->SetTitle("z (mm)");
            } else if(project == 'y') {
                z = z_min + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * (z_max - z_min);
                histogram->GetYaxis()->SetTitle("z (mm)");
                histogram_x->GetYaxis()->SetTitle("z (mm)");
                histogram_y->GetYaxis()->SetTitle("z (mm)");
                histogram_z->GetYaxis()->SetTitle("z (mm)");
            } else {
                y = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0 +
                    ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().y();
                histogram->GetYaxis()->SetTitle("y (mm)");
                histogram_x->GetYaxis()->SetTitle("y (mm)");
                histogram_y->GetYaxis()->SetTitle("y (mm)");
                histogram_z->GetYaxis()->SetTitle("y (mm)");
            }

            // Get field strength from detector - directly convert to double to fill root histograms
            auto field = detector_->getElectricField(ROOT::Math::XYZPoint(x, y, z));
            auto field_strength = static_cast<double>(Units::convert(std::sqrt(field.Mag2()), "V/cm"));
            auto field_x_strength = static_cast<double>(Units::convert(field.x(), "V/cm"));
            auto field_y_strength = static_cast<double>(Units::convert(field.y(), "V/cm"));
            auto field_z_strength = static_cast<double>(Units::convert(field.z(), "V/cm"));
            // Fill the main histogram
            if(project == 'x') {
                histogram->Fill(y, z, field_strength);
                histogram_x->Fill(y, z, field_x_strength);
                histogram_y->Fill(y, z, field_y_strength);
                histogram_z->Fill(y, z, field_z_strength);
            } else if(project == 'y') {
                histogram->Fill(x, z, field_strength);
                histogram_x->Fill(x, z, field_x_strength);
                histogram_y->Fill(x, z, field_y_strength);
                histogram_z->Fill(x, z, field_z_strength);
            } else {
                histogram->Fill(x, y, field_strength);
                histogram_x->Fill(x, y, field_x_strength);
                histogram_y->Fill(x, y, field_y_strength);
                histogram_z->Fill(x, y, field_z_strength);
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
    histogram1D->Write();
}

/**
 * @brief Check if the detector matches the file header
 */
void ElectricFieldReaderModule::check_detector_match(std::array<double, 3> dimensions,
                                                     std::pair<double, double> thickness_domain,
                                                     std::array<double, 2> field_scale) {
    auto xpixsz = dimensions[0];
    auto ypixsz = dimensions[1];
    auto thickness = dimensions[2];

    auto model = detector_->getModel();
    // Do a several checks with the detector model
    if(model != nullptr) {
        // Check field dimension in z versus the requested thickness domain:
        auto eff_thickness = thickness_domain.second - thickness_domain.first;
        if(std::fabs(thickness - eff_thickness) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Thickness of electric field is " << Units::display(thickness, "um")
                         << " but the depleted region is " << Units::display(eff_thickness, "um");
        }

        // Check the field extent along the pixel pitch in x and y:
        if(std::fabs(xpixsz - field_scale[0]) > std::numeric_limits<double>::epsilon() ||
           std::fabs(ypixsz - field_scale[1]) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Electric field size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"})
                         << ") but current configuration results in an field area of ("
                         << Units::display(field_scale[0], {"um", "mm"}) << ","
                         << Units::display(field_scale[1], {"um", "mm"}) << ")" << std::endl
                         << "The size of the area to which the electric field is applied can be changes using the "
                            "field_scale parameter.";
        }
    }
}
