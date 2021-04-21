/**
 * @file
 * @brief Parameters of a hexagonal pixel detector model
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H
#define ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "DetectorModel.hpp"

#include "HybridPixelDetectorModel.hpp"

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a hexagonal pixel detector. This a model where the sensor is bump-bonded to the chip
     */
    class HexagonalPixelDetectorModel : public HybridPixelDetectorModel {
    public:
        /**
         * @brief Constructs the hexagonal pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit HexagonalPixelDetectorModel(std::string type, const ConfigReader& reader)
            : HybridPixelDetectorModel(std::move(type), reader) {
            auto config = reader.getHeaderConfiguration();

        }

	/**
	 * Tasneem: override getPixelIndex() function for Hexagonal Pixel:
	 * @brief Get new index after coordinate transformation for hexagonal pixel shape
	 * @return Index of a pixel
	 */

        ROOT::Math::XYVector getPixelIndex(const ROOT::Math::XYZPoint & position) const override 
	{
		double sizex = getPixelSize().x();
                double radius = std::sqrt(3)/3.0 * sizex;
                double height = 0.5 * radius;
                //cout << "Size_x " << sizex << ", radius " << radius << ", height: "
                //     << height << endl;
                // Charge positions.
                double posx = position.x();
                double posy = position.y();
                // Positions in (q,r) reference.
                int r = static_cast<int> ((posy + height) / (3.0 * height));
                //int q = static_cast<int> (std::round((posx + ((-r + 1) * 0.5 * sizex)) / sizex));
                int q = 0;
                if(r%2 == 0)
                	q = static_cast<int> ((posx + (0.5 * sizex)) / sizex);
                else
                        q = static_cast<int> (posx / sizex);
                // Charge between 3 hexagonal pixels.
                double y1 = posy + height - (r * 3.0 * height);
                if(y1 > (2.0 * height))
                        {
                                double x1 = posx - (q * sizex);
                                if(r%2 == 1)
                                x1 = x1 - (0.5 * sizex);
                                double xlimit = 0.5 * sizex * ((3.0 * height) - y1) / height;
                                if(r%2 == 0)
                	        {
                                        if(x1 > xlimit)
                                                {r++;}
                                        else if(x1 < -xlimit)
                                                { q--; r++;}
                                }
                                else
                                {
                                        if(x1 > xlimit)
                                                {q++; r++;}
                                        else if(x1 < -xlimit)
                                                {r++;}
                                }
                        }
                //std::cout << "Charge position in x,y: " << posx << ", "
                //        << posy << ", Idem in q,r: " << q << ", " << r << std::endl;
                //getchar();

                // Nearest pixel for hexagonal pixels.
                double xpixel = q;
                double ypixel = r;

                return {xpixel, ypixel};
	}

        /**
	 * Tasneem override getGridSize() function for Hexagonal Pixel:
         * @brief Get size of the grid
         * @return Size of the grid
	 * Grid size in y direction = 3.0 * height * numberPixel.y, where height is defined as above in the getPixelIndex() function!
         */
        ROOT::Math::XYZVector getGridSize() const {
            return {getNPixels().x() * getPixelSize().x(), getNPixels().y() * 3.0 * 0.5 * std::sqrt(3)/3.0 * getPixelSize().x(), 0};
        }

    };
} // namespace allpix

#endif /* ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H */
