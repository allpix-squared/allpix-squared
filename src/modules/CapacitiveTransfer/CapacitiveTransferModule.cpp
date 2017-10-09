/*
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
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

CapacitiveTransferModule::CapacitiveTransferModule(Configuration config,
                                                   Messenger* messenger,
                                                   std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)) {
    // Save detector model
    model_ = detector_->getModel();

    // Require propagated deposits for single detector
    messenger->bindSingle(this, &CapacitiveTransferModule::propagated_message_, MsgFlags::REQUIRED);
}

void CapacitiveTransferModule::init() {

    // Reading coupling matrix from config file
    if(config_.has("coupling_matrix")) {
        // TODO
        // relative_coupling = config_.get<Matrix>("coupling_matrix");
    }
    // Reading file with coupling matrix
    else if(config_.has("matrix_file")) {
        LOG(TRACE) << "Reading cross-coupling matrix file " << config_.get<std::string>("matrix_file");
        std::ifstream input_file(config_.getPath("matrix_file", true), std::ifstream::in);
        if(!input_file.good()) {
            LOG(ERROR) << "Matrix file not found";
        }

        double dummy = -1;
        std::string file_line;
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
    }
    // If no coupling matrix is provided
    else {
        LOG(ERROR)
            << "Cross-coupling was not defined. Provide a coupling matrix file or a coupling matrix in the config file.";
    }

    LOG(DEBUG) << matrix_cols << "x" << matrix_rows << " Capacitance matrix imported";
    // TODO
    // LOG(DEBUG) << relaive_coupling;;
    //

    if(config_.has("nominal_gap")) {
        nominal_gap = config_.get<double>("nominal_gap");
    }
    Eigen::Vector3d origin(0, 0, nominal_gap);

    if(config_.has("tilt_center")) {
        center[0] = config_.get<ROOT::Math::XYPoint>("tilt_center").x() * model_->getPixelSize().x();
        center[1] = config_.get<ROOT::Math::XYPoint>("tilt_center").y() * model_->getPixelSize().y();
        origin = Eigen::Vector3d(center[0], center[1], nominal_gap);
    }

    if(config_.has("scan_file")) {
        TFile* root_file = new TFile(config_.getPath("scan_file", true).c_str());
        for(int i = 1; i < 10; i++) {
            capacitances[i - 1] = static_cast<TGraph*>(root_file->Get(Form("Pixel_%i", i)));
        }
        root_file->Delete();
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

    plane = Eigen::Hyperplane<double, 3>(rotated_normal, origin);

    if(config_.get<bool>("output_plots")) {
        LOG(TRACE) << "Creating output plots";

        // Create histograms if needed
        auto pixel_grid = model_->getNPixels();
        Eigen::Vector3d point1(0, 0, 0);
        Eigen::Vector3d point2(pixel_grid.x() * model_->getPixelSize().x(), pixel_grid.y() * model_->getPixelSize().y(), 0);
        Eigen::Vector3d point1_proj = plane.projection(point1);
        Eigen::Vector3d point2_proj = plane.projection(point2);

        gap_distribution = new TH1D("gap_distribution",
                                    "Gap;Gap[nn];#Entries",
                                    100,
                                    static_cast<double>(Units::convert(point1_proj[2], "um")),
                                    static_cast<double>(Units::convert(point2_proj[2], "um")));
        gap_map = new TH2D("gap_map",
                           "Gap;pixel x;pixel y",
                           pixel_grid.x(),
                           -0.5,
                           pixel_grid.x() - 0.5,
                           pixel_grid.y(),
                           -0.5,
                           pixel_grid.y() - 0.5);
        capacitance_map = new TH2D("capacitance_map",
                                   "Gap;pixel x;pixel y",
                                   pixel_grid.x(),
                                   -0.5,
                                   pixel_grid.x() - 0.5,
                                   pixel_grid.y(),
                                   -0.5,
                                   pixel_grid.y() - 0.5);
    }
}

double CapacitiveTransferModule::gap(Pixel::Index pixel) {

    auto local_x = pixel.x() * model_->getPixelSize().x();
    auto local_y = pixel.y() * model_->getPixelSize().y();

    Eigen::Vector3d pixel_point(local_x, local_y, 0);
    Eigen::Vector3d pixel_projection = plane.projection(pixel_point);
    pixel_gap = pixel_projection[2];

    gap_distribution->Fill(static_cast<double>(Units::convert(pixel_gap, "um")));
    gap_map->SetBinContent(
        static_cast<int>(pixel.x()), static_cast<int>(pixel.y()), static_cast<double>(Units::convert(pixel_gap, "um")));

    capacitance_map->SetBinContent(
        static_cast<int>(pixel.x()), static_cast<int>(pixel.y()), capacitances[4]->Eval(pixel_gap, 0, "S"));

    return pixel_gap;
}

void CapacitiveTransferModule::run(unsigned int) {

    // Find corresponding pixels for all propagated charges
    LOG(TRACE) << "Transferring charges to pixels";
    unsigned int transferred_charges_count = 0;
    std::map<Pixel::Index, std::pair<double, std::vector<const PropagatedCharge*>>, pixel_cmp> pixel_map;
    for(auto& propagated_charge : propagated_message_->getData()) {
        auto position = propagated_charge.getLocalPosition();
        // Ignore if outside depth range of implant
        // FIXME This logic should be improved
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

        for(size_t row = 0; row < matrix_rows; row++) {
            for(size_t col = 0; col < matrix_cols; col++) {

                // Ignore if out of pixel grid
                if((xpixel + static_cast<int>(col - static_cast<size_t>(std::floor(matrix_cols / 2)))) < 0 ||
                   (xpixel + static_cast<int>(col - static_cast<size_t>(std::floor(matrix_cols / 2)))) >=
                       model_->getNPixels().x() ||
                   (ypixel + static_cast<int>(row - static_cast<size_t>(std::floor(matrix_rows / 2)))) < 0 ||
                   (ypixel + static_cast<int>(row - static_cast<size_t>(std::floor(matrix_rows / 2)))) >=
                       model_->getNPixels().y()) {
                    LOG(DEBUG) << "Skipping set of " << propagated_charge.getCharge() * relative_coupling[col][row]
                               << " propagated charges at " << propagated_charge.getLocalPosition()
                               << " because their nearest pixel (" << xpixel << "," << ypixel
                               << ") is outside the pixel matrix";
                    continue;
                }

                Pixel::Index pixel_index(
                    static_cast<unsigned int>(xpixel + static_cast<int>(col) - std::floor(matrix_cols / 2)),
                    static_cast<unsigned int>(ypixel + static_cast<int>(row) - std::floor(matrix_rows / 2)));

                // Update statistics
                unique_pixels_.insert(pixel_index);

                double neighbour_charge;
                double ccpd_factor;
                if(config_.has("scan_file")) {
                    ccpd_factor = capacitances[row * 3 + col]->Eval(gap(pixel_index), 0, "S") /
                                  capacitances[4]->Eval(nominal_gap, 0, "S");
                } else {
                    ccpd_factor = relative_coupling[col][row];
                }

                transferred_charges_count += static_cast<unsigned int>(propagated_charge.getCharge() * ccpd_factor);
                neighbour_charge = propagated_charge.getCharge() * ccpd_factor;
                if(col == static_cast<size_t>(std::floor(matrix_cols / 2)) &&
                   row == static_cast<size_t>(std::floor(matrix_rows / 2))) {
                    LOG(DEBUG) << "Set of " << propagated_charge.getCharge() * ccpd_factor << " charges brought to pixel "
                               << pixel_index;
                } else {
                    LOG(DEBUG) << "Set of " << propagated_charge.getCharge() * ccpd_factor
                               << " charges brought to neighbour " << col << "," << row << " pixel " << pixel_index
                               << "with cross-coupling of " << ccpd_factor * 100 << "%";
                }

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
    messenger_->dispatchMessage(this, pixel_message);
}

void CapacitiveTransferModule::finalize() {
    // Print statistics
    LOG(INFO) << "Transferred total of " << total_transferred_charges_ << " charges to " << unique_pixels_.size()
              << " different pixels";

    if(config_.get<bool>("output_plots")) {
        gap_distribution->Write();
        gap_map->Write();
        capacitance_map->Write();
        if(config_.has("scan_file")) {
            for(int i = 1; i < 10; i++) {
                capacitances[i - 1]->Write(Form("Pixel_%i", i));
            }
        }
    }
}
