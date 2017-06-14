#include "ElectricFieldReaderInitModule.hpp"

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
#include "core/geometry/HybridPixelDetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

// use the allpix namespace within this file
using namespace allpix;

// constructor to load the module
ElectricFieldReaderInitModule::ElectricFieldReaderInitModule(Configuration config,
                                                             Messenger*,
                                                             std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), detector_(std::move(detector)) {}

// init method that reads the electric field from the file
void ElectricFieldReaderInitModule::init() {
    try {
        // get field
        LOG(TRACE) << "Reading electric field file";
        auto field_data = get_by_file_name(config_.getPath("file_name", true), *detector_.get());

        // set detector field
        detector_->setElectricField(field_data.first, field_data.second);

        // produce debug histograms if needed
        if(config_.get<bool>("output_plots", false)) {
            LOG(TRACE) << "Creating output plots";

            auto steps = config_.get<size_t>("output_plots_steps", 5000);
            auto project = config_.get<char>("output_plots_project", 'x');

            if(project != 'x' && project != 'y' && project != 'z') {
                throw InvalidValueError(config_, "output_plots_project", "can only project on x, y or z axis");
            }

            auto model = detector_->getModel();

            double min1, max1;
            double min2, max2;
            if(project == 'x') {
                min1 = model->getSensorMinY();
                max1 = model->getSensorMinY() + model->getSensorSize().y();
                min2 = model->getSensorMinZ();
                max2 = model->getSensorMinZ() + model->getSensorSize().z();
            } else if(project == 'y') {
                min1 = model->getSensorMinX();
                max1 = model->getSensorMinX() + model->getSensorSize().x();
                min2 = model->getSensorMinZ();
                max2 = model->getSensorMinZ() + model->getSensorSize().z();
            } else {
                min1 = model->getSensorMinX();
                max1 = model->getSensorMinX() + model->getSensorSize().x();
                min2 = model->getSensorMinY();
                max2 = model->getSensorMinY() + model->getSensorSize().y();
            }

            auto histogram = new TH2F("field",
                                      ("Electric field for " + detector_->getName()).c_str(),
                                      static_cast<int>(steps),
                                      min1,
                                      max1,
                                      static_cast<int>(steps),
                                      min2,
                                      max2);

            double x = 0, y = 0, z = 0;
            if(project == 'x') {
                x = model->getSensorMinX() +
                    config_.get<double>("output_plots_projection_percentage", 0.5) * model->getSensorSize().x();
            } else if(project == 'y') {
                y = model->getSensorMinY() +
                    config_.get<double>("output_plots_projection_percentage", 0.5) * model->getSensorSize().y();
            } else {
                z = model->getSensorMinZ() +
                    config_.get<double>("output_plots_projection_percentage", 0.5) * model->getSensorSize().z();
            }
            for(size_t j = 0; j < steps; ++j) {
                if(project == 'x') {
                    y = model->getSensorMinY() +
                        ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().y();
                } else if(project == 'y') {
                    x = model->getSensorMinX() +
                        ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().x();
                } else {
                    x = model->getSensorMinX() +
                        ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().x();
                }
                for(size_t k = 0; k < steps; ++k) {
                    if(project == 'x') {
                        z = model->getSensorMinZ() +
                            ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().z();
                    } else if(project == 'y') {
                        z = model->getSensorMinZ() +
                            ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().z();
                    } else {
                        y = model->getSensorMinY() +
                            ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * model->getSensorSize().y();
                    }

                    // get field strength and fill histogram
                    auto field_strength = std::sqrt(detector_->getElectricField(ROOT::Math::XYZPoint(x, y, z)).Mag2());

                    // fill histogram
                    if(project == 'x') {
                        histogram->Fill(y, z, field_strength);
                    } else if(project == 'y') {
                        histogram->Fill(x, z, field_strength);
                    } else {
                        histogram->Fill(x, y, field_strength);
                    }
                }
            }

            // write histogram
            histogram->Write();
        }
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::runtime_error& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::bad_alloc& e) {
        throw InvalidValueError(config_, "file_name", "file too large");
    }
}

// check if the detector matches the file
inline static void check_detector_match(Detector& detector, double thickness, double xpixsz, double ypixsz) {
    auto model = std::dynamic_pointer_cast<HybridPixelDetectorModel>(detector.getModel());
    // do a few simple checks for pixel detector models
    if(model != nullptr) {
        if(std::fabs(thickness - model->getSensorSize().z()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Thickness of sensor in file is " << Units::display(thickness, "um")
                         << " but in the model it is " << Units::display(model->getSensorSize().z(), "um");
        }
        if(std::fabs(xpixsz - model->getPixelSizeX()) > std::numeric_limits<double>::epsilon() ||
           std::fabs(ypixsz - model->getPixelSizeY()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Pixel size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"}) << ") but in the model it is ("
                         << Units::display(model->getPixelSizeX(), {"um", "mm"}) << ","
                         << Units::display(model->getPixelSizeY(), {"um", "mm"}) << ")";
        }
    }
}

// get the electric field from file name (reusing the same if catched earlier)
std::map<std::string, ElectricFieldReaderInitModule::FieldData> ElectricFieldReaderInitModule::field_map_;
ElectricFieldReaderInitModule::FieldData ElectricFieldReaderInitModule::get_by_file_name(const std::string& file_name,
                                                                                         Detector& detector) {
    // search in cache (NOTE: the path reached here is always a canonical name)
    auto iter = field_map_.find(file_name);
    if(iter != field_map_.end()) {
        // FIXME: check detector match here as well
        return iter->second;
    }

    // load file
    std::ifstream file(file_name);

    std::string header;
    std::getline(file, header);

    LOG(TRACE) << "Header of file " << file_name << " is " << header;

    // read the header
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

    // check if electric field matches chip
    check_detector_match(detector, thickness, xpixsz, ypixsz);

    if(file.fail()) {
        throw std::runtime_error("invalid data or unexpected end of file");
    }
    auto field = std::make_shared<std::vector<double>>();
    field->resize(xsize * ysize * zsize * 3);

    // loop through all the field data
    for(size_t i = 0; i < xsize * ysize * zsize; ++i) {
        if(file.eof()) {
            throw std::runtime_error("unexpected end of file");
        }

        // get index of electric field
        size_t xind, yind, zind;
        file >> xind >> yind >> zind;

        if(file.fail() || xind > xsize || yind > ysize || zind > zsize) {
            throw std::runtime_error("invalid data");
        }
        xind--;
        yind--;
        zind--;

        // loop through components of electric field
        for(size_t j = 0; j < 3; ++j) {
            double input;
            file >> input;

            // set the electric field at a position
            (*field)[xind * ysize * zsize * 3 + yind * zsize * 3 + zind * 3 + j] = Units::get(input, "V/cm");
        }
    }

    return std::make_pair(field, std::array<size_t, 3>{{xsize, ysize, zsize}});
}
