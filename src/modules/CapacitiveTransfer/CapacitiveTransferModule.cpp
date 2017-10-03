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
#include <Eigen/Geometry>

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
    gap_distribution = new TH1D("gap_distribution", "Gap;Gap[nn];#Entries", 50, -50., 50.);
    gap_map = new TH2D("gap_map", "Gap;pixel x;pixel y", 64, 0, 63, 64, 0, 63);
    gap_root_file = new TFile("gap_map.root", "RECREATE");

    // Reading file with coupling matrix
    if(config_.has("matrix_file")) {
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
    // Reading coupling matrix from config file
    if(config_.has("coupling_matrix")) {
        // TODO
        // relative_coupling = config_.get<Matrix>("coupling_matrix");
    }
    // If no coupling matrix is provided
    if(!config_.has("matrix_file") && !config_.has("coupling_matrix")) {
        LOG(ERROR)
            << "Cross-coupling was not defined. Provide a coupling matrix file or a coupling matrix in the config file.";
    }

    LOG(DEBUG) << matrix_cols << "x" << matrix_rows << " Capacitance matrix imported";
    // TODO
    // LOG(DEBUG) << relaive_coupling;;
}

double CapacitiveTransferModule::gap(int xpixel, int ypixel) {
    int center[2] = {0, 0};
    double angles[2] = {0.0, 0.0};
    double gap = 1;

    if(config_.has("chip_angle")) {
        auto angles_temp = config_.get<ROOT::Math::XYPoint>("chip_angle");
        angles[0] = angles_temp.x();
        angles[1] = angles_temp.y();
        if(config_.has("gradient_center")) {
            auto center_temp = config_.get<ROOT::Math::XYPoint>("gradient_center");
            center[0] = static_cast<int>(center_temp.x());
            center[1] = static_cast<int>(center_temp.y());
        }

        Eigen::Quaternion<double> quaternion =
            Eigen::AngleAxisd(angles[0], Eigen::Vector3d::UnitX()) * Eigen::AngleAxisd(angles[1], Eigen::Vector3d::UnitY());
        quaternion.w() = 0;
        Eigen::Matrix3d rotation = quaternion.toRotationMatrix();

        Eigen::Vector3d pixel_point(xpixel * 25e-6, ypixel * 25e-6, 0);
        Eigen::Vector3d origin(center[0] * 25e-6, center[1] * 25e-6, 0);
        Eigen::Vector3d normal(0, 0, 1);
        Eigen::Vector3d rotated_normal = rotation * normal;

        Eigen::Hyperplane<double, 3> plane(rotated_normal, origin);
        Eigen::Vector3d point_plane = plane.projection(pixel_point);
        gap = point_plane[2] * 1000;
    }
    LOG(DEBUG) << "===> GAP: " << gap << " nm";
    gap_distribution->Fill(gap);
    gap_map->SetBinContent(xpixel, ypixel, gap);

    return gap;
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

                gap(xpixel, ypixel);

                transferred_charges_count +=
                    static_cast<unsigned int>(propagated_charge.getCharge() * relative_coupling[col][row]);
                double neighbour_charge = propagated_charge.getCharge() * relative_coupling[col][row];

                if(col == static_cast<size_t>(std::floor(matrix_cols / 2)) &&
                   row == static_cast<size_t>(std::floor(matrix_rows / 2))) {
                    LOG(DEBUG) << "Set of " << propagated_charge.getCharge() * relative_coupling[col][row]
                               << " charges brought to pixel " << pixel_index;
                } else {
                    LOG(DEBUG) << "Set of " << propagated_charge.getCharge() * relative_coupling[col][row]
                               << " charges brought to neighbour " << col << "," << row << " pixel " << pixel_index
                               << "with cross-coupling of " << relative_coupling[col][row] * 100 << "%";
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

    gap_root_file->cd();
    gap_distribution->Write();
    gap_map->Write();
}
