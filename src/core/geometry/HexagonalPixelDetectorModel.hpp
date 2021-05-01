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

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a hexagonal pixel detector. This a model where the sensor is bump-bonded to the chip
     */
    class HexagonalPixelDetectorModel : public DetectorModel {
    public:
        /**
         * @brief Constructs the hexagonal pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit HexagonalPixelDetectorModel(std::string type, const ConfigReader& reader)
            : DetectorModel(std::move(type), reader) {
            auto config = reader.getHeaderConfiguration();

            // Excess around the chip from the pixel grid
            auto default_chip_excess = config.get<double>("chip_excess", 0);
            setChipExcessTop(config.get<double>("chip_excess_top", default_chip_excess));
            setChipExcessBottom(config.get<double>("chip_excess_bottom", default_chip_excess));
            setChipExcessLeft(config.get<double>("chip_excess_left", default_chip_excess));
            setChipExcessRight(config.get<double>("chip_excess_right", default_chip_excess));

            // Set bump parameters
            setBumpCylinderRadius(config.get<double>("bump_cylinder_radius"));
            setBumpHeight(config.get<double>("bump_height"));
            setBumpSphereRadius(config.get<double>("bump_sphere_radius", 0));
            setBumpOffset(config.get<ROOT::Math::XYVector>("bump_offset", {0, 0}));
        }

	/**
	 * override getPixelIndex() function for Hexagonal Pixel:
	 * @brief Get new index after coordinate transformation for hexagonal pixel shape
	 * @return Index of a pixel
	 */

        ROOT::Math::XYVector getPixelIndex(const ROOT::Math::XYZPoint & position) const override {
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
	 * getGridSize() function for Hexagonal Pixel:
         * @brief Get size of the grid
         * @return Size of the grid
	 * Grid size in y direction = 3.0 * height * numberPixel.y, where height is defined as above in the getPixelIndex() function!
         */
        ROOT::Math::XYZVector getGridSize() const {
            return {getNPixels().x() * getPixelSize().x(), getNPixels().y() * 3.0 * 0.5 * std::sqrt(3)/3.0 * getPixelSize().x(), 0};
        }

        /**
         * @brief Get size of the chip
         * @return Size of the chip
         *
         * Calculated from \ref DetectorModel::getGridSize "pixel grid size", chip excess and chip thickness
         */
        ROOT::Math::XYZVector getChipSize() const override {
            ROOT::Math::XYZVector excess_thickness(
                (chip_excess_.at(1) + chip_excess_.at(3)), (chip_excess_.at(0) + chip_excess_.at(2)), chip_thickness_);
            return getGridSize() + excess_thickness;
        }
        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * The center of the chip as given by \ref DetectorModel::getChipCenter() with extra offset for bump bonds.
         */
        ROOT::Math::XYZPoint getChipCenter() const override {
            ROOT::Math::XYZVector offset((chip_excess_.at(1) - chip_excess_.at(3)) / 2.0,
                                         (chip_excess_.at(0) - chip_excess_.at(2)) / 2.0,
                                         getSensorSize().z() / 2.0 + getChipSize().z() / 2.0 + getBumpHeight());
            return getCenter() + offset;
        }

        /**
         * @brief Set the excess at the top of the chip (positive y-coordinate)
         * @param val Chip top excess
         */
        void setChipExcessTop(double val) { chip_excess_.at(0) = val; }
        /**
         * @brief Set the excess at the right of the chip (positive x-coordinate)
         * @param val Chip right excess
         */
        void setChipExcessRight(double val) { chip_excess_.at(1) = val; }
        /**
         * @brief Set the excess at the bottom of the chip (negative y-coordinate)
         * @param val Chip bottom excess
         */
        void setChipExcessBottom(double val) { chip_excess_.at(2) = val; }
        /**
         * @brief Set the excess at the left of the chip (negative x-coordinate)
         * @param val Chip left excess
         */
        void setChipExcessLeft(double val) { chip_excess_.at(3) = val; }

        /**
         * @brief Return all layers of support
         * @return List of all the support layers
         *
         * The center of the support is adjusted to take the bump bonds into account
         */
        std::vector<SupportLayer> getSupportLayers() const override {
            auto ret_layers = DetectorModel::getSupportLayers();

            for(auto& layer : ret_layers) {
                if(layer.location_ == "chip") {
                    layer.center_.SetZ(layer.center_.z() + getBumpHeight());
                }
            }

            return ret_layers;
        }

        /**
         * @brief Get the center of the bump bonds in local coordinates
         * @return Center of the bump bonds
         *
         * The bump bonds are aligned with the grid with an optional XY-offset. The z-offset is calculated with the sensor
         * and chip offsets taken into account.
         */
        virtual ROOT::Math::XYZPoint getBumpsCenter() const {
            ROOT::Math::XYZVector offset(
                bump_offset_.x(), bump_offset_.y(), getSensorSize().z() / 2.0 + getBumpHeight() / 2.0);
            return getCenter() + offset;
        }
        /**
         * @brief Get the radius of the sphere of every individual bump bond (union solid with cylinder)
         * @return Radius of bump bond sphere
         */
        double getBumpSphereRadius() const { return bump_sphere_radius_; }
        /**
         * @brief Set the radius of the sphere of every individual bump bond  (union solid with cylinder)
         * @param val Radius of bump bond sphere
         */
        void setBumpSphereRadius(double val) { bump_sphere_radius_ = val; }
        /**
         * @brief Get the radius of the cylinder of every individual bump bond  (union solid with sphere)
         * @return Radius of bump bond cylinder
         */
        double getBumpCylinderRadius() const { return bump_cylinder_radius_; }
        /**
         * @brief Set the radius of the cylinder of every individual bump bond  (union solid with sphere)
         * @param val Radius of bump bond cylinder
         */
        void setBumpCylinderRadius(double val) { bump_cylinder_radius_ = val; }
        /**
         * @brief Get the height of the bump bond cylinder, determining the offset between sensor and chip
         * @return Height of the bump bonds
         */
        double getBumpHeight() const { return bump_height_; }
        /**
         * @brief Set the height of the bump bond cylinder, determining the offset between sensor and chip
         * @param val Height of the bump bonds
         */
        void setBumpHeight(double val) { bump_height_ = val; }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }

    private:
        std::array<double, 4> chip_excess_{};

        double bump_sphere_radius_{};
        double bump_height_{};
        ROOT::Math::XYVector bump_offset_;
        double bump_cylinder_radius_{};
    };
} // namespace allpix

#endif /* ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H */
