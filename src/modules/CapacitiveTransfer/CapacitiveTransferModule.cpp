/*
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CapacitiveTransferModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/config/exceptions.h"
#include "core/geometry/HybridPixelDetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"

#include <Eigen/Core>

#include "objects/PixelCharge.hpp"

using namespace allpix;

CapacitiveTransferModule::CapacitiveTransferModule(Configuration& config,
                                                   Messenger* messenger,
                                                   std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();

    model_ = detector_->getModel();
    config_.setDefault("output_plots", 0);
    config_.setDefault("cross_coupling", 1);
    config_.setDefault("nominal_gap", 0.0);
    config_.setDefault("minimum_gap", config_.get<double>("nominal_gap"));

    // Require propagated deposits for single detector
    messenger_->bindSingle<PropagatedChargeMessage>(this, MsgFlags::REQUIRED);
}

void CapacitiveTransferModule::init() {

    if(config_.count({"coupling_matrix", "coupling_file", "coupling_scan_file"}) > 1) {
        throw InvalidCombinationError(
            config_, {"coupling_matrix", "coupling_file", "coupling_scan_file"}, "More than one coupling input defined");
    } else if(config_.has("coupling_matrix")) {
        relative_coupling = config_.getMatrix<double>("coupling_matrix");
        max_row = static_cast<unsigned int>(relative_coupling.size());
        max_col = static_cast<unsigned int>(relative_coupling[0].size());

        if(config_.get<bool>("output_plots")) {
            LOG(TRACE) << "Creating output plots";

            // Create histograms if needed
            coupling_map = CreateHistogram<TH2D>("coupling_map",
                                                 "Coupling;pixel x;pixel y",
                                                 static_cast<int>(max_col),
                                                 -0.5,
                                                 static_cast<int>(max_col) - 0.5,
                                                 static_cast<int>(max_row),
                                                 -0.5,
                                                 static_cast<int>(max_row) - 0.5);

            for(size_t col = 0; col < max_col; col++) {
                for(size_t row = 0; row < max_row; row++) {
                    coupling_map->SetBinContent(
                        static_cast<int>(col + 1), static_cast<int>(row + 1), relative_coupling[max_row - row - 1][col]);
                }
            }
        }

        LOG(STATUS) << max_col << "x" << max_row << " coupling matrix imported from config file";

    } else if(config_.has("coupling_file")) {
        LOG(TRACE) << "Reading cross-coupling matrix file " << config_.get<std::string>("coupling_file");
        std::ifstream input_file(config_.getPath("coupling_file", true), std::ifstream::in);
        if(!input_file.good()) {
            throw InvalidValueError(config_, "coupling_file", "Matrix file not found");
        }

        double dummy = -1;
        std::string file_line;
        matrix_rows = 0;
        while(getline(input_file, file_line)) {
            std::stringstream mystream(file_line);
            matrix_cols = 0;
            while(mystream >> dummy) {
                matrix_cols++;
            }
            matrix_rows++;
        }
        input_file.clear();
        input_file.seekg(0, std::ios::beg);

        relative_coupling.resize(matrix_rows);
        for(auto& el : relative_coupling) {
            el.resize(matrix_cols);
        }

        max_col = matrix_cols;
        max_row = matrix_rows;

        size_t row = matrix_rows - 1, col = 0;
        while(getline(input_file, file_line)) {
            std::stringstream mystream(file_line);
            col = 0;
            while(mystream >> dummy) {
                relative_coupling[col][row] = dummy;
                col++;
            }
            row--;
        }
        input_file.close();

        if(config_.get<bool>("output_plots")) {
            LOG(TRACE) << "Creating output plots";

            // Create histograms if needed
            coupling_map = CreateHistogram<TH2D>("coupling_map",
                                                 "Coupling;pixel x;pixel y",
                                                 static_cast<int>(max_col),
                                                 -0.5,
                                                 static_cast<int>(max_col) - 0.5,
                                                 static_cast<int>(max_row),
                                                 -0.5,
                                                 static_cast<int>(max_row) - 0.5);

            for(col = 0; col < max_col; col++) {
                for(row = 0; row < max_row; row++) {
                    coupling_map->SetBinContent(
                        static_cast<int>(col + 1), static_cast<int>(row + 1), relative_coupling[col][row]);
                }
            }
        }

        LOG(STATUS) << matrix_cols << "x" << matrix_rows << " capacitance matrix imported from file "
                    << config_.get<std::string>("coupling_file");

    } else if(config_.has("coupling_scan_file")) {
        auto* root_file = new TFile(config_.getPath("coupling_scan_file", true).c_str());
        if(root_file->IsZombie()) {
            throw InvalidValueError(config_, "coupling_scan_file", "ROOT file is corrupted. Please, check it");
        }
        for(int i = 1; i < 10; i++) {
            capacitances[i - 1] = static_cast<TGraph*>(root_file->Get(Form("Pixel_%i", i)));
            if(capacitances[i - 1]->IsZombie()) {
                throw InvalidValueError(
                    config_, "coupling_scan_file", "ROOT TGraphs couldn't be imported. Please, check it");
            }
            capacitances[i - 1]->SetBit(TGraph::kIsSortedX);
        }
        matrix_cols = 3;
        matrix_rows = 3;
        if(config_.get<int>("cross_coupling") == 1) {
            max_col = 3;
            max_row = 3;
        } else {
            LOG(STATUS) << "Cross-coupling (neighbour charge transfer) disabled";
            max_col = 1;
            max_row = 1;
        }

        root_file->Delete();

        if(config_.has("minimum_gap")) {
            minimum_gap = config_.get<double>("minimum_gap");
        }
        if(config_.has("nominal_gap")) {
            nominal_gap = config_.get<double>("nominal_gap");
        }
        Eigen::Vector3d origin(0, 0, minimum_gap);

        if(config_.has("tilt_center")) {
            center[0] = config_.get<ROOT::Math::XYPoint>("tilt_center").x() * model_->getPixelSize().x();
            center[1] = config_.get<ROOT::Math::XYPoint>("tilt_center").y() * model_->getPixelSize().y();
            origin = Eigen::Vector3d(center[0], center[1], minimum_gap);
        }

        Eigen::Vector3d rotated_normal(0, 0, 1);
        if(config_.has("chip_angle")) {
            angles[0] = config_.get<ROOT::Math::XYPoint>("chip_angle").x();
            angles[1] = config_.get<ROOT::Math::XYPoint>("chip_angle").y();

            if(angles[0] != 0.0) {
                auto rotation_x = Eigen::AngleAxisd(angles[0], Eigen::Vector3d::UnitX()).toRotationMatrix();
                rotated_normal = rotation_x * rotated_normal;
            }
            if(angles[1] != 0.0) {
                auto rotation_y = Eigen::AngleAxisd(angles[1], Eigen::Vector3d::UnitY()).toRotationMatrix();
                rotated_normal = rotation_y * rotated_normal;
            }
        }

        normalization = 1 / (capacitances[4]->Eval(static_cast<double>(Units::convert(nominal_gap, "um")), nullptr, "S"));
        plane = Eigen::Hyperplane<double, 3>(rotated_normal, origin);

        if(config_.get<bool>("output_plots")) {
            LOG(TRACE) << "Creating output plots";

            // Create histograms if needed
            auto xpixels = static_cast<int>(model_->getNPixels().x());
            auto ypixels = static_cast<int>(model_->getNPixels().y());

            gap_map = CreateHistogram<TH2D>(
                "gap_map", "Gap;pixel x;pixel y", xpixels, -0.5, xpixels, ypixels, -0.5, ypixels - 0.5);

            capacitance_map = CreateHistogram<TH2D>(
                "capacitance_map", "Capacitance;pixel x;pixel y", xpixels, -0.5, xpixels, ypixels, -0.5, ypixels - 0.5);

            relative_capacitance_map = CreateHistogram<TH2D>("relative_capacitance_map",
                                                             "Relative Capacitance;pixel x;pixel y",
                                                             xpixels,
                                                             -0.5,
                                                             xpixels,
                                                             ypixels,
                                                             -0.5,
                                                             ypixels - 0.5);

            for(int col = 0; col < xpixels; col++) {
                for(int row = 0; row < ypixels; row++) {
                    auto local_x = col * model_->getPixelSize().x();
                    auto local_y = row * model_->getPixelSize().y();

                    Eigen::Vector3d pixel_point(local_x, local_y, 0);
                    Eigen::Vector3d pixel_projection = plane.projection(pixel_point);
                    auto pixel_gap = pixel_projection[2];

                    gap_map->Fill(col, row, static_cast<double>(Units::convert(pixel_gap, "um")));
                    capacitance_map->Fill(
                        col, row, capacitances[4]->Eval(static_cast<double>(Units::convert(pixel_gap, "um")), nullptr, "S"));
                    relative_capacitance_map->Fill(
                        col,
                        row,
                        capacitances[4]->Eval(static_cast<double>(Units::convert(pixel_gap, "um")), nullptr, "S") /
                            capacitances[4]->Eval(static_cast<double>(Units::convert(nominal_gap, "um")), nullptr, "S"));
                }
            }

            LOG(STATUS) << "Using " << config_.getPath("coupling_scan_file", true).c_str()
                        << " ROOT file as input for the capacitance vs pixel gaps";
        }
    } else {
        throw ModuleError(
            "Capacitive coupling was not defined. Please, check the README file for configuration options or use "
            "the SimpleTransfer module.");
    }
}

void CapacitiveTransferModule::run(Event* event) {
    auto propagated_message = messenger_->fetchMessage<PropagatedChargeMessage>(this, event);

    // Find corresponding pixels for all propagated charges
    LOG(TRACE) << "Transferring charges to pixels";
    unsigned int transferred_charges_count = 0;
    std::map<Pixel::Index, std::pair<double, std::vector<const PropagatedCharge*>>> pixel_map;
    for(const auto& propagated_charge : propagated_message->getData()) {
        auto position = propagated_charge.getLocalPosition();
        // Ignore if outside depth range of implant
        if(std::fabs(position.z() - (model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0)) >
           config_.get<double>("max_depth_distance")) {
            LOG(DEBUG) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << propagated_charge.getLocalPosition() << " because their local position is not in implant range";
            continue;
        }

        // Find the nearest pixel
        auto xpixel = static_cast<int>(std::round(position.x() / model_->getPixelSize().x()));
        auto ypixel = static_cast<int>(std::round(position.y() / model_->getPixelSize().y()));
        LOG(DEBUG) << "Hit at pixel " << xpixel << ", " << ypixel;

        Pixel::Index pixel_index;
        double neighbour_charge = 0;
        double ccpd_factor = 0;

        Eigen::Vector3d pixel_point;
        Eigen::Vector3d pixel_projection;

        for(size_t row = 0; row < max_row; row++) {
            for(size_t col = 0; col < max_col; col++) {
                if(config_.get<int>("cross_coupling") == 0) {
                    col = static_cast<size_t>(std::floor(matrix_cols / 2));
                    row = static_cast<size_t>(std::floor(matrix_rows / 2));
                }

                auto xcoord = xpixel + static_cast<int>(col - static_cast<size_t>(std::floor(matrix_cols / 2)));
                auto ycoord = ypixel + static_cast<int>(row - static_cast<size_t>(std::floor(matrix_rows / 2)));

                // Ignore if out of pixel grid
                if(!detector_->isWithinPixelGrid(xcoord, ycoord)) {
                    LOG(DEBUG) << "Skipping set of propagated charges at " << propagated_charge.getLocalPosition()
                               << " because their nearest pixel (" << xpixel << "," << ypixel
                               << ") is outside the pixel matrix";
                    continue;
                }

                pixel_index = Pixel::Index(static_cast<unsigned int>(xcoord), static_cast<unsigned int>(ycoord));

                if(config_.has("coupling_scan_file")) {
                    double local_x = pixel_index.x() * model_->getPixelSize().x();
                    double local_y = pixel_index.y() * model_->getPixelSize().y();
                    pixel_point = Eigen::Vector3d(local_x, local_y, 0);
                    pixel_projection = plane.projection(pixel_point);
                    auto pixel_gap = pixel_projection[2];

                    ccpd_factor = capacitances[row * 3 + col]->Eval(
                                      static_cast<double>(Units::convert(pixel_gap, "um")), nullptr, "S") *
                                  normalization;
                } else if(config_.has("coupling_file")) {
                    ccpd_factor = relative_coupling[col][row];
                } else if(config_.has("coupling_matrix")) {
                    ccpd_factor = relative_coupling[max_row - row - 1][col];
                } else {
                    ccpd_factor = 1;
                    LOG(ERROR) << "This shouldn't happen. Transferring 100% of detected charge";
                }

                // Update statistics
                {
                    std::lock_guard<std::mutex> lock{stats_mutex_};
                    unique_pixels_.insert(pixel_index);
                }
                transferred_charges_count += static_cast<unsigned int>(propagated_charge.getCharge() * ccpd_factor);
                neighbour_charge =
                    static_cast<double>(propagated_charge.getSign() * propagated_charge.getCharge()) * ccpd_factor;

                LOG(DEBUG) << "Set of " << propagated_charge.getCharge() * ccpd_factor << " charges brought to neighbour "
                           << col << "," << row << " pixel " << pixel_index << "with cross-coupling of " << ccpd_factor * 100
                           << "%";

                // Add the pixel the list of hit pixels
                pixel_map[pixel_index].first += neighbour_charge;
                pixel_map[pixel_index].second.emplace_back(&propagated_charge);
            }
        }
    }

    // Create pixel charges
    LOG(TRACE) << "Combining charges at same pixel";
    std::vector<PixelCharge> pixel_charges;
    for(auto& pixel_index_charge : pixel_map) {
        double charge = pixel_index_charge.second.first;

        // Get pixel object from detector
        auto pixel = detector_->getPixel(pixel_index_charge.first.x(), pixel_index_charge.first.y());
        pixel_charges.emplace_back(pixel, charge, pixel_index_charge.second.second);
        LOG(DEBUG) << "Set of " << charge << " charges combined at " << pixel.getIndex();
    }

    // Writing summary and update statistics
    LOG(INFO) << "Transferred " << transferred_charges_count << " charges to " << pixel_map.size() << " pixels";
    total_transferred_charges_ += transferred_charges_count;

    // Dispatch message of pixel charges
    auto pixel_message = std::make_shared<PixelChargeMessage>(pixel_charges, detector_);
    messenger_->dispatchMessage(this, pixel_message, event);
}

void CapacitiveTransferModule::finalize() {
    // Print statistics
    LOG(INFO) << "Transferred total of " << total_transferred_charges_ << " charges to " << unique_pixels_.size()
              << " different pixels";

    if(config_.get<bool>("output_plots")) {
        if(config_.has("coupling_scan_file")) {
            gap_map->Write();
            capacitance_map->Write();
            relative_capacitance_map->Write();
            for(int i = 1; i < 10; i++) {
                capacitances[i - 1]->Write(Form("Pixel_%i", i));
            }
        } else {
            coupling_map->Write();
        }
    }
}
