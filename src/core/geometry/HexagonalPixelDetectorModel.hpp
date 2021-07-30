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

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "DetectorModel.hpp"

namespace allpix {
    class HexagonalPixelDetectorModel : public DetectorModel {

    private:
       //static double offset;
       //int xpixel;
       //int ypixel;
    public:

		explicit HexagonalPixelDetectorModel(std::string type, const ConfigReader& reader) : DetectorModel(std::move(type), reader) {}

		/*
		getPixelIndex function: takes in the local coordinates of the hit position and outputs the x,y pixel indices of the pixel grid. 
								Here, a normal hexagon pixel and rectangular grid are assumed with index (0,0) being the bottom left most pixel.
								For now, part of the hexagon pixels at the edge of the rectangular grid is assumed to be out of grid
								Note: out of grid pixels set to indices (-1,-1) which are then subsequently filtered out by isWithinPixelGrid() 
		*/
		std::pair<int, int> getPixelIndex(const ROOT::Math::XYZPoint& position) const override {
			double pixelsize = getPixelSize().x(); //defined as side to side length (pitch)
			double side = pixelsize / std::sqrt(3);
			double minor_radius = pixelsize / 2;
			double posx = position.x() + minor_radius; 
			double posy = position.y() + side;
			int x_check = static_cast<int>(std::floor(posx  / minor_radius));
			int y_check = static_cast<int>(std::floor(posy / (side / 2)));
			double offset;	
		
			int xpixel = -1;
			int ypixel = -1;	

			switch(y_check % 6){
				case 0: {
					if(x_check % 2 == 0){
						offset = (3 * side * std::floor(posy / (3 * side))) + ((std::sqrt(3) / 3) * ((x_check + 1) * minor_radius));
						if(posy > (-(std::sqrt(3) / 3) * posx + offset)){
							xpixel = static_cast<int>(std::ceil(posx / pixelsize));
							ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
						}else if(posy < (-(std::sqrt(3) / 3) * posx + offset)){
							xpixel = static_cast<int>(std::floor(posx / pixelsize));
							ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)));	
						} 							
					}else if(x_check % 2 == 1){ 
						offset = (3 * side * std::floor(posy / (3 * side))) - ((std::sqrt(3) / 3) * (x_check * minor_radius));
						if(posy > ((std::sqrt(3) / 3) * posx + offset)){
							xpixel = static_cast<int>(std::ceil(posx / pixelsize));
							ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1); 
						}else if(posy < ((std::sqrt(3) / 3) * posx + offset)){
							xpixel = static_cast<int>(std::ceil(posx / pixelsize));
							ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)));
						}						
					}
					break;
				}
				case 1: case 2: {
					xpixel = static_cast<int>(std::ceil(posx / pixelsize));
					ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
					break;
				}
				case 3: {
					if(x_check % 2 == 0){
						offset = side * (3 * std::floor(posy / (3 * side)) + 2) - (std::sqrt(3) / 3) * (minor_radius * (x_check + 1));
						if(posy > ((std::sqrt(3) / 3) * posx + offset)){
							xpixel = static_cast<int>(std::ceil((posx - minor_radius) / pixelsize));
							ypixel = static_cast<int>(2 * std::floor(posy / (3 * side))  + 2);
						}else if(posy < ((std::sqrt(3) / 3) * posx + offset)){
							xpixel = static_cast<int>(std::ceil(posx / pixelsize));
							ypixel = static_cast<int>(2 * std::floor(posy / (3 * side))  + 1);
						}			
					}else if(x_check % 2 == 1){
						offset = side * (3 * std::floor(posy / (3 * side)) + 2) + ((std::sqrt(3) / 3) * (minor_radius * x_check));
						if(posy > (-(std::sqrt(3) / 3) * posx + offset)){
							xpixel = static_cast<int>(std::ceil((posx - minor_radius) / pixelsize));
							ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 2);
						}else if(posy < (-(std::sqrt(3) / 3) * posx + offset)){
							xpixel = static_cast<int>(std::ceil(posx / pixelsize));
							ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
						}
					}
					break;
				}			
				case 4: case 5: {
					xpixel = static_cast<int>(std::ceil((posx - minor_radius) / pixelsize));
					ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 2);
					break;				
				}
			}
			//calculation done based on 1 as first index, but then shift to 0 here to match convention defined in user manual
			return {xpixel - 1, ypixel - 1}; 
		} //End of getPixelIndex function
	
		/*
		getGridsize function: outputs the dimensions of the gridsize along x and y direction 
		x dimension: length from side to side length of the outer pixels of the 1st row
		y dimension: length from corner to corner of the outer pixels of the 1st column
		*/
		ROOT::Math::XYZVector getGridSize() const override {
	    	double x_gridsize = getNPixels().x() * getPixelSize().x();
			double y_gridsize = 0;
	    	unsigned int pixely_num = getNPixels().y();
			double side = getPixelSize().x() / std::sqrt(3);
			if((pixely_num % 2) == 1){
				y_gridsize = (((pixely_num - 1) / 2) * 3 * side) + (2 * side);
				//std::cout << "Gridsize Y: " << y_gridsize << std::endl;
			}else if(pixely_num % 2 == 0){
				y_gridsize = ((pixely_num / 2) * 3 * side) + (side / 2);
	    	}
	    	return ROOT::Math::XYZVector(x_gridsize, y_gridsize, 0);
		} //End of getGridSize function
    }; //End of HexagonalPixelDetectorModel class
} //End of namespace allpix squared
#endif /* ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H */
