#include "ElectricFieldReaderInitModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/config/InvalidValueError.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/utils/log.h"

// use the allpix namespace within this file
using namespace allpix;

// set the name of the module
const std::string ElectricFieldReaderInitModule::name = "ElectricFieldReaderInit";

// constructor to load the module
ElectricFieldReaderInitModule::ElectricFieldReaderInitModule(Configuration config,
                                                             Messenger*,
                                                             std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(std::move(detector)) {}

// run method that does the main computations for the module
void ElectricFieldReaderInitModule::run() {
    try {
        std::shared_ptr<std::vector<double>> field =
            get_by_file_name(config_.get<std::string>("file_name"), *detector_.get());
    } catch(std::invalid_argument&) {
        throw InvalidValueError(config_, "file_name", "specified file does not exist");
    }
}

// check if the detector matches the file
inline static void check_detector_match(Detector& detector, double thickness, int npixx, int npixy) {
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector.getModel());
    // do a few simple checks for pixel detector models
    if(model != nullptr) {
        if(std::fabs(thickness - model->getSensorSizeZ()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "thickness of sensor is not equal to the value given in the model of " << detector.getName();
        }
        if(npixx != model->getNPixelsX() || npixy != model->getNPixelsY()) {
            LOG(WARNING) << "amount of pixels of sensor is not equal to the value given in the model of "
                         << detector.getName();
        }
    }
}

// get the electric field from file name (reusing the same if catched earlier)
std::map<std::string, std::shared_ptr<std::vector<double>>> ElectricFieldReaderInitModule::field_map_;
std::shared_ptr<std::vector<double>> ElectricFieldReaderInitModule::get_by_file_name(std::string name, Detector& detector) {
    std::string fullpath = realpath(name.c_str(), nullptr);

    // search in cache
    auto iter = field_map_.find(fullpath);
    if(iter != field_map_.end()) {
        // FIXME: check detector match here as well
        return iter->second;
    }

    // check if file is correct
    std::ifstream file(fullpath);
    if(!file.good()) {
        throw std::invalid_argument("file not found");
    }

    std::string header;
    std::getline(std::cin, header);

    LOG(DEBUG) << "header of file " << name << " is " << header;

    double tmp;
    std::cin >> tmp >> tmp;        // ignore the init seed and cluster length
    std::cin >> tmp >> tmp >> tmp; // ignore the incident pion direction
    std::cin >> tmp >> tmp >> tmp; // ignore the magnetic field (specify separately)
    double thickness, npixx, npixy;
    std::cin >> thickness >> npixx >> npixy;
    std::cin >> tmp >> tmp >> tmp >> tmp; // ignore temperature, flux, rhe (?) and new_drde (?)
    int xsize, ysize, zsize;
    std::cin >> xsize >> ysize >> zsize;
    std::cin >> tmp;

    check_detector_match(detector, thickness, static_cast<int>(std::round(npixx)), static_cast<int>(std::round(npixy)));

    LOG(DEBUG) << " size is " << xsize << " " << ysize << " " << zsize << std::endl;

    /*
        fscanf( ifp,"%d %d", initseed, runsize );

        printf( "default random number seed %d, events/run %d\n",
          *initseed, *runsize );

        // incident pion direction:

        fscanf( ifp,"%f %f %f", &pidir[0], &pidir[1], &pidir[2] );

        printf( "turn, tilt, 1 = %f, %f, %f\n", pidir[0], pidir[1], pidir[2] );

        // magnetic field:

        fscanf( ifp,"%f %f %f", &bfield.f[0], &bfield.f[1], &bfield.f[2] );
        bfield.f[3] = 0.;

        printf( "Bfield = %f, %f, %f, %f\n", bfield.f[0], bfield.f[1], bfield.f[2], bfield.f[3] );

        // parameters:

        fscanf( ifp,"%f %f %f %f %f %f %d %d %d %d %d",
          thick, xsize, ysize, temp, flux, rhe, new_drde, &npixx, &npixy, &npixz, fullgrid );

        // AB: If this fails need to make efield array bigger!
        assert(npixx <= 150 && npixy <= 100 && npixz <= 300);

        printf( "thickness = %f, x/y sizes = %f/%f, temp = %f, fluence = %f, rhe = %f, new_drde = %d, fullgrid = %d \n",
          *thick, *xsize, *ysize, *temp, *flux, *rhe, *new_drde, *fullgrid );

        printf( "x/y/z field array dimensions = %d/%d/%d\n", npixx, npixy, npixz );
  */

    return nullptr;
}
