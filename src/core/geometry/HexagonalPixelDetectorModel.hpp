/**
 * @file
 * @brief Parameters of a hexagonal pixel detector model
 *
 * @copyright Copyright (c) 2017-2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H
#define ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H

namespace allpix {
    class HexagonalPixelDetectorModel : public DetectorModel {
    public:
        explicit HexagonalPixelDetectorModel(std::string type, const ConfigReader& reader)
            : DetectorModel(std::move(type), reader) {}

        /**
         * @brief Return X,Y indices of a pixel corresponding to a local position in a hexagonal sensor grid.
         * @return X,Y pixel indices
         */
        std::pair<int, int> getPixelIndex(const ROOT::Math::XYZPoint& position) const override {
            double pixelsize = getPixelSize().x(); // defined as side to side length (pitch)
            double side = pixelsize / std::sqrt(3);
            double minor_radius = pixelsize / 2;
            double posx = position.x() + minor_radius;
            double posy = position.y() + side;

            int x_modulus = static_cast<int>(std::floor(posx / minor_radius));
            int y_modulus = static_cast<int>(std::floor(((posy / (3 * side)) - std::floor(posy / (3 * side))) * 6));

            double offset;

            int xpixel = static_cast<int>(std::ceil(posx / pixelsize));
            int ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);

            switch(y_modulus % 6) {
            case 0: {
                if(x_modulus % 2 == 0) {
                    offset =
                        (3 * side * std::floor(posy / (3 * side))) + ((std::sqrt(3) / 3) * ((x_modulus + 1) * minor_radius));
                    if(posy < (-(std::sqrt(3) / 3) * posx + offset)) {
                        xpixel -= 1;
                        ypixel -= 1;
                    }
                } else if(x_modulus % 2 == 1 || x_modulus % 2 == -1) {
                    offset = (3 * side * std::floor(posy / (3 * side))) - ((std::sqrt(3) / 3) * (x_modulus * minor_radius));
                    if(posy < ((std::sqrt(3) / 3) * posx + offset)) {
                        ypixel -= 1;
                    }
                }
                break;
            }
            case 1:
            case 2: {
                break;
            }
            case 3: {
                if(x_modulus % 2 == 0) {
                    offset = side * (3 * std::floor(posy / (3 * side)) + 2) -
                             (std::sqrt(3) / 3) * (minor_radius * (x_modulus + 1));
                    if(posy > ((std::sqrt(3) / 3) * posx + offset)) {
                        xpixel -= 1;
                        ypixel += 1;
                    }
                } else if(x_modulus % 2 == 1 || x_modulus % 2 == -1) {
                    offset =
                        side * (3 * std::floor(posy / (3 * side)) + 2) + ((std::sqrt(3) / 3) * (minor_radius * x_modulus));
                    if(posy > (-(std::sqrt(3) / 3) * posx + offset)) {
                        ypixel += 1;
                    }
                }
                break;
            }
            case 4:
            case 5: {
                if(x_modulus % 2 == 0) {
                    xpixel -= 1;
                    ypixel += 1;
                } else if(x_modulus % 2 == 1 || x_modulus % 2 == -1) {
                    ypixel += 1;
                }
                break;
            }
            default:
                // Bugged values (if there is) are set as (-1,-1)
                xpixel = -1;
                ypixel = -1;
            }
            return {xpixel - 1, ypixel - 1};
        } // End of getPixelIndex function

        /**
         * @brief Return gridsize along X,Y of a hexagonal sensor grid.
         * @return X,Y gridlength length in mm
         */
        ROOT::Math::XYZVector getGridSize() const override {
            double x_gridsize = getNPixels().x() * getPixelSize().x();
            double y_gridsize = 0;
            unsigned int pixely_num = getNPixels().y();
            double side = getPixelSize().x() / std::sqrt(3);
            if((pixely_num % 2) == 1) {
                y_gridsize = (((pixely_num - 1) / 2) * 3 * side) + (2 * side);
            } else if(pixely_num % 2 == 0) {
                y_gridsize = ((pixely_num / 2) * 3 * side) + (side / 2);
            }
            return ROOT::Math::XYZVector(x_gridsize, y_gridsize, 0);
        } // End of getGridSize function
    };    // End of HexagonalPixelDetectorModel class
} // namespace allpix
#endif /* ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H */
