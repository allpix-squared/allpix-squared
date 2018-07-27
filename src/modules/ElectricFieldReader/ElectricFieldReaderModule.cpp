/**
 * @file
 * @brief Implementation of module to read electric fields
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
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
#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

ElectricFieldReaderModule::ElectricFieldReaderModule(Configuration& config, Messenger*, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {
    // NOTE use voltage as a synonym for bias voltage
    config_.setAlias("bias_voltage", "voltage");
}

void ElectricFieldReaderModule::init() {
    ElectricFieldType type = ElectricFieldType::GRID;

    // Check field strength
    auto field_model = config_.get<std::string>("model");
    if((field_model == "constant" || field_model == "linear") &&
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
    if(field_model == "init") {
        ElectricFieldReaderModule::FieldData field_data;
        field_data = read_init_field(thickness_domain);
        detector_->setElectricFieldGrid(std::get<0>(field_data), std::get<1>(field_data), thickness_domain);
    } else if(field_model == "constant") {
        LOG(TRACE) << "Adding constant electric field";
        type = ElectricFieldType::CONSTANT;

        auto field_z = config_.get<double>("bias_voltage") / getDetector()->getModel()->getSensorSize().z();
        LOG(INFO) << "Set constant electric field with magnitude " << Units::display(field_z, {"V/um", "V/mm"});
        ElectricFieldFunction function = [field_z](const ROOT::Math::XYZPoint&) {
            return ROOT::Math::XYZVector(0, 0, -field_z);
        };
        detector_->setElectricFieldFunction(function, thickness_domain, type);
    } else if(field_model == "linear") {
        LOG(TRACE) << "Adding linear electric field";
        type = ElectricFieldType::LINEAR;

        // Get depletion voltage, defaults to bias voltage:
        auto depletion_voltage = config_.get<double>("depletion_voltage", config_.get<double>("bias_voltage"));

        LOG(INFO) << "Setting linear electric field from " << Units::display(config_.get<double>("bias_voltage"), "V")
                  << " bias voltage and " << Units::display(depletion_voltage, "V") << " depletion voltage";
        ElectricFieldFunction function = get_linear_field_function(depletion_voltage, thickness_domain);
        detector_->setElectricFieldFunction(function, thickness_domain, type);
    } else {
        throw InvalidValueError(config_, "model", "model should be 'linear', 'constant' or 'init'");
    }

    // Produce histograms if needed
    if(config_.get<bool>("output_plots", false)) {
        create_output_plots();
    }
}

ElectricFieldFunction ElectricFieldReaderModule::get_linear_field_function(double depletion_voltage,
                                                                           std::pair<double, double> thickness_domain) {
    LOG(TRACE) << "Calculating function for the linear electric field.";
    // We always deplete from the implants:
    auto bias_voltage = std::fabs(config_.get<double>("bias_voltage"));
    depletion_voltage = std::fabs(depletion_voltage);
    // But the direction of the field depends on the applied voltage:
    auto direction = std::signbit(config_.get<double>("bias_voltage"));
    double eff_thickness = thickness_domain.second - thickness_domain.first;
    // Reduce the effective thickness of the sensor if voltage is below full depletion
    if(bias_voltage < depletion_voltage) {
        eff_thickness *= sqrt(bias_voltage / depletion_voltage);
        depletion_voltage = bias_voltage;
    }
    LOG(TRACE) << "Effective thickness of the electric field: " << Units::display(eff_thickness, {"um", "mm"});
    return [bias_voltage, depletion_voltage, direction, eff_thickness, thickness_domain](const ROOT::Math::XYZPoint& pos) {
        double z_rel = thickness_domain.second - pos.z();
        double field_z = std::max(0.0,
                                  (bias_voltage - depletion_voltage) / eff_thickness +
                                      2 * (depletion_voltage / eff_thickness) * (1 - z_rel / eff_thickness));
        return ROOT::Math::XYZVector(0, 0, (direction ? -1 : 1) * field_z);
    };
}

/**
 * The field read from the INIT format are shared between module instantiations using the static
 * ElectricFieldReaderModuleget_by_file_name method.
 */
ElectricFieldReaderModule::FieldData ElectricFieldReaderModule::read_init_field(std::pair<double, double> thickness_domain) {
    try {
        LOG(TRACE) << "Fetching electric field from init file";

        // Get field from file
        auto field_data = get_by_file_name(config_.getPath("file_name", true));

        // Check if electric field matches chip
        check_detector_match(std::get<2>(field_data), thickness_domain);

        LOG(INFO) << "Set electric field with " << std::get<1>(field_data).at(0) << "x" << std::get<1>(field_data).at(1)
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
        model->setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>(1, 1));
    }
    // Use either full sensor axis or only depleted region
    double z_min = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0;
    double z_max = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;

    // Determine minimum and maximum index depending on projection axis
    double min1, max1;
    double min2, max2;
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

    // Create 2D histogram
    auto histogram = new TH2F("field_magnitude",
                              "electric field magnitude",
                              static_cast<int>(steps),
                              min1,
                              max1,
                              static_cast<int>(steps),
                              min2,
                              max2);
    histogram->SetMinimum(0);

    // Create 1D histogram
    auto histogram1D = new TH1F(
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

    // Find the electric field at every index
    for(size_t j = 0; j < steps; ++j) {
        if(project == 'x') {
            y = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().y();
            histogram->GetXaxis()->SetTitle("y (mm)");
        } else if(project == 'y') {
            x = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().x();
            histogram->GetXaxis()->SetTitle("x (mm)");
        } else {
            x = model->getSensorCenter().x() - model->getSensorSize().x() / 2.0 +
                ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().x();
            histogram->GetXaxis()->SetTitle("x (mm)");
        }
        for(size_t k = 0; k < steps; ++k) {
            if(project == 'x') {
                z = z_min + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * (z_max - z_min);
                histogram->GetYaxis()->SetTitle("z (mm)");
            } else if(project == 'y') {
                z = z_min + ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * (z_max - z_min);
                histogram->GetYaxis()->SetTitle("z (mm)");
            } else {
                y = model->getSensorCenter().y() - model->getSensorSize().y() / 2.0 +
                    ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().y();
                histogram->GetYaxis()->SetTitle("y (mm)");
            }

            // Get field strength from detector
            auto field = detector_->getElectricField(ROOT::Math::XYZPoint(x, y, z));
            auto field_strength = Units::convert(std::sqrt(field.Mag2()), "V/cm");
            auto field_z_strength = Units::convert(field.z(), "V/cm");
            // Fill the main histogram
            if(project == 'x') {
                histogram->Fill(y, z, static_cast<double>(field_strength));
            } else if(project == 'y') {
                histogram->Fill(x, z, static_cast<double>(field_strength));
            } else {
                histogram->Fill(x, y, static_cast<double>(field_strength));
            }
            // Fill the 1d histogram
            if(j == steps / 2) {
                histogram1D->Fill(z, static_cast<double>(field_z_strength));
            }
        }
    }

    // Write the histogram to module file
    histogram->Write();
    histogram1D->Write();
}

/**
 * @brief Check if the detector matches the file header
 */
void ElectricFieldReaderModule::check_detector_match(std::array<double, 3> dimensions,
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
            LOG(WARNING) << "Thickness of electric field is " << Units::display(thickness, "um")
                         << " but the depleted region is " << Units::display(eff_thickness, "um");
        }
        // Check the field extent along the pixel pitch in x and y:
        if(std::fabs(xpixsz - model->getPixelSize().x()) > std::numeric_limits<double>::epsilon() ||
           std::fabs(ypixsz - model->getPixelSize().y()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Electric field size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"}) << ") but the pixel pitch is ("
                         << Units::display(model->getPixelSize().x(), {"um", "mm"}) << ","
                         << Units::display(model->getPixelSize().y(), {"um", "mm"}) << ")";
        }
    }
}

std::map<std::string, ElectricFieldReaderModule::FieldData> ElectricFieldReaderModule::field_map_;
ElectricFieldReaderModule::FieldData ElectricFieldReaderModule::get_by_file_name(const std::string& file_name) {
    // Search in cache (NOTE: the path reached here is always a canonical name)
    auto iter = field_map_.find(file_name);
    if(iter != field_map_.end()) {
        LOG(INFO) << "Using cached electric field data";
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

    if(file.fail()) {
        throw std::runtime_error("invalid data or unexpected end of file");
    }
    auto field = std::make_shared<std::vector<double>>();
    field->resize(xsize * ysize * zsize * 3);

    // Loop through all the field data
    for(size_t i = 0; i < xsize * ysize * zsize; ++i) {
        if(i % 100 == 0) {
            LOG_PROGRESS(INFO, "read_init") << "Reading electric field data: " << (100 * i / (xsize * ysize * zsize)) << "%";
        }

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

    FieldData field_data = std::make_tuple(
        field, std::array<size_t, 3>{{xsize, ysize, zsize}}, std::array<double, 3>{{xpixsz, ypixsz, thickness}});

    // Store the parsed field data for further reference:
    field_map_[file_name] = field_data;
    return field_data;
}
