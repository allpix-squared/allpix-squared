#include "ElectricFieldReaderInitModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/config/InvalidValueError.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

// use the allpix namespace within this file
using namespace allpix;

// set the name of the module
const std::string ElectricFieldReaderInitModule::name = "ElectricFieldReaderInit";

// constructor to load the module
ElectricFieldReaderInitModule::ElectricFieldReaderInitModule(Configuration config,
                                                             Messenger*,
                                                             std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(std::move(detector)) {}

// init method that reads the electric field from the file
void ElectricFieldReaderInitModule::init() {
    try {
        // get field
        LOG(INFO) << "Reading electric field file";
        auto field_data = get_by_file_name(config_.get<std::string>("file_name"), *detector_.get());

        // set detector field
        detector_->setElectricField(field_data.first, field_data.second);
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::runtime_error& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::bad_alloc& e) {
        throw InvalidValueError(config_, "file_name", "file too large");
    }
}

// check if the detector matches the file
inline static void check_detector_match(Detector& detector, double thickness, int npixx, int npixy) {
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector.getModel());
    // do a few simple checks for pixel detector models
    if(model != nullptr) {
        if(std::fabs(thickness - model->getSensorSizeZ()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "thickness of sensor in file is " << thickness << " but in the model of " << detector.getName()
                         << " it is " << model->getSensorSizeZ();
        }
        if(npixx != model->getNPixelsX() || npixy != model->getNPixelsY()) {
            LOG(WARNING) << "number of pixels is (" << npixx << "," << npixy << ") but in the model of "
                         << detector.getName() << " it is (" << model->getNPixelsX() << "," << model->getNPixelsY() << ")";
        }
    }
}

// get the electric field from file name (reusing the same if catched earlier)
std::map<std::string, ElectricFieldReaderInitModule::FieldData> ElectricFieldReaderInitModule::field_map_;
ElectricFieldReaderInitModule::FieldData ElectricFieldReaderInitModule::get_by_file_name(const std::string& file_name,
                                                                                         Detector& detector) {
    char* path_ptr = realpath(file_name.c_str(), nullptr);
    if(path_ptr == nullptr) {
        throw std::invalid_argument("file not found");
    }
    std::string fullpath(path_ptr);
    free(static_cast<void*>(path_ptr));

    // search in cache
    auto iter = field_map_.find(fullpath);
    if(iter != field_map_.end()) {
        // FIXME: check detector match here as well
        return iter->second;
    }

    // check if file is correct
    std::ifstream file(fullpath);

    std::string header;
    std::getline(file, header);

    LOG(DEBUG) << "header of file " << file_name << " is " << header;

    std::string tmp;
    file >> tmp >> tmp;        // ignore the init seed and cluster length
    file >> tmp >> tmp >> tmp; // ignore the incident pion direction
    file >> tmp >> tmp >> tmp; // ignore the magnetic field (specify separately)
    double thickness, npixx, npixy;
    file >> thickness >> npixx >> npixy;
    thickness = Units::get(thickness, "um");
    file >> tmp >> tmp >> tmp >> tmp; // ignore temperature, flux, rhe (?) and new_drde (?)
    size_t xsize, ysize, zsize;
    file >> xsize >> ysize >> zsize;
    file >> tmp;

    check_detector_match(detector, thickness, static_cast<int>(std::round(npixx)), static_cast<int>(std::round(npixy)));

    if(file.fail()) {
        throw std::runtime_error("invalid data or unexpected end of file");
    }
    auto field = std::make_shared<std::vector<double>>();
    field->resize(xsize * ysize * zsize * 3);

    for(size_t i = 0; i < xsize * ysize * zsize; ++i) {
        if(file.eof()) {
            throw std::runtime_error("unexpected end of file");
        }

        size_t xind, yind, zind;
        file >> xind >> yind >> zind;

        if(file.fail() || xind > xsize || yind > ysize || zind > zsize) {
            throw std::runtime_error("invalid data");
        }
        xind--;
        yind--;
        zind--;

        for(size_t j = 0; j < 3; ++j) {
            double input;
            file >> input;

            (*field)[xind * ysize * zsize * 3 + yind * zsize * 3 + zind * 3 + j] = Units::get(input, "V/cm");
        }
    }

    return std::make_pair(field, std::array<size_t, 3>{{xsize, ysize, zsize}});
}
