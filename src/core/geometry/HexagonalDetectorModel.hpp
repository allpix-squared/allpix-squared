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
       //double offset;
       //int xpixel;
       //int ypixel;
    public:
        ROOT::Math::XYVector getPixelIndex(const ROOT::Math::XYZPoint & position) const override {
			//outside gridsize, to skip (this will be outside the function, to change in respective modules)
			//how is the gridsize defined? Shouldn't there be a condition that evaluates if total number of pixels is even or odd	
	    double x_gridsize = getGridSize().x(); //length from 0 to side length from outer pixel of 1st row
	    double y_gridsize = getGridSize().y(); //length from corner to corner of outer pixels of 1st row
	    double pixelsize = getPixelSize().x(); //defined as side to side length (pitch)
	    double posx = position.x();
	    double posy = position.y();
	    double side = pixelsize / std::sqrt(3);
	    double minor_radius = pixelsize / 2;
	    int out_of_grid = -1;		
	    int x_check = static_cast<int>(std::floor(posx / minor_radius));
	    int y_check = static_cast<int>(std::floor(posy / (side / 2)));
		
	    int xpixel = out_of_grid;
	    int ypixel = out_of_grid;	

	    //Check if it is outside the pixel grid
	    if(x_check < 0 || x_check >= (x_gridsize / minor_radius)){
		return ROOT::Math::XYVector(xpixel, ypixel);
	    }else if(y_check < 0 || y_check >= (y_gridsize / (side / 2))){
		return ROOT::Math::XYVector(xpixel, ypixel);
	    }

	    if(x_check >= 1 && (x_gridsize - minor_radius) >= posx){
	    //switch(x_check){
	    //    case x_check >= 1 && (x_gridsize - minor_radius) >= posx:
	        switch(y_check % 6){
		    case 0:
		        if(x_check % 2 == 0){
			    double offset = (3 * side * std::floor(posy / (3 * side))) + ((std::sqrt(3) / 3) * ((x_check + 1) * minor_radius));
			    if(posy > (-(std::sqrt(3) / 3) * posx + offset)){
				xpixel = static_cast<int>(std::ceil(posx / pixelsize));
				ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
			    }else if(posy < (-(std::sqrt(3) / 3) * posx + offset)){
				xpixel = static_cast<int>(std::floor(posx / pixelsize));
				ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)));	
			    } 							
			} else if(x_check % 2 == 1){ 
		   	    double offset = (3 * side * std::floor(posy / (3 * side))) - ((std::sqrt(3) / 3) * (x_check * minor_radius));
		            if(posy > ((std::sqrt(3) / 3) * posx + offset)){
			        xpixel = static_cast<int>(std::ceil(posx / pixelsize));
		      	        ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1); 
			    }else if(posy < ((std::sqrt(3) / 3) * posx + offset)){
				xpixel = static_cast<int>(std::ceil(posx / pixelsize));
				ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)));
			    }							
			}
			break;
		    case 1: case 2:
			xpixel = static_cast<int>(std::ceil(posx / pixelsize));
                        ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
			break;	
		    case 3:
			if(x_check % 2 == 0){
			    double offset = side * (3 * std::floor(posy / (3 * side)) + 2) - (std::sqrt(3) / 3) * (minor_radius * (x_check + 1));
			    if(posy > ((std::sqrt(3) / 3) * posx + offset)){
				xpixel = static_cast<int>(std::ceil((posx - minor_radius) / pixelsize));
				ypixel = static_cast<int>(2 * std::floor(posy / (3 * side))  + 2);
			    }else if(posy < ((std::sqrt(3) / 3) * posx + offset)){
				xpixel = static_cast<int>(std::ceil(posx / pixelsize));
				ypixel = static_cast<int>(2 * std::floor(posy / (3 * side))  + 1);
			    }			
			}else if(x_check % 2 == 1){
			    double offset = side * (3 * std::floor(posy / (3 * side)) + 2) + ((std::sqrt(3) / 3) * (minor_radius * x_check));
			    if(posy > (-(std::sqrt(3) / 3) * posx + offset)){
			        xpixel = static_cast<int>(std::ceil((posx - minor_radius) / pixelsize));
				ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 2);
			    }else if(posy < (-(std::sqrt(3) / 3) * posx + offset)){
				xpixel = static_cast<int>(std::ceil(posx / pixelsize));
				ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
			    }
			}
			break;			
		    case 4: case 5:
			xpixel = static_cast<int>(std::ceil((posx - minor_radius) / pixelsize));
			ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 2);
			break;				
		    }
		    break;
	    }else if(x_check == 0){
		switch(y_check % 6){
		    case 0:
			double offset = side * (0.5 + (3 * std::floor(posy / (3 * side))));
			if(posy > (-(std::sqrt(3) / 3) * posx + offset)){
			    xpixel = 1;
			    ypixel = static_cast<int>(2 * std::floor(posy / (3 * side))  + 1);
			}else if(posy < (-(std::sqrt(3) / 3) * (posx) + offset)){
			    xpixel = out_of_grid;
			    ypixel = out_of_grid;
	 		}		    
			break;
		    case 1: case 2:
			xpixel = 1;
			ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
			break;
		    case 3:
			double offset = side * (1.5 + (3 * std::floor(posy / (3 * side))));
			if(posy > ((std::sqrt(3) / 3) * posx + offset)){
			    xpixel = out_of_grid;
			    ypixel = out_of_grid;
			}else if(posy < ((std::sqrt(3) / 3) * posx + offset)){
			    xpixel = 1;
			    ypixel = static_cast<int>(2 * std::floor(posy / (3 * side))  + 1);
			}
			break;
		    case 4: case 5:
			xpixel = out_of_grid;
			ypixel = out_of_grid;
			break;
		    } 
		    break;		
	    }else if(x_check == ((x_gridsize / minor_radius) - 1)){
	        switch(y_check % 6){
		    case 0:
			double offset = ((3 * side) * (std::floor(posy / (3 * side)))) - ((std::sqrt(3) / 3) * (x_gridsize - minor_radius));
              		if(posy > ((std::sqrt(3) / 3)*(posx) + offset)){
			    xpixel = static_cast<int>(std::ceil(posx / pixelsize));
			    ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
			}else if(posy < ((std::sqrt(3) / 3)*(posx) + offset)){
			    xpixel = out_of_grid;
			    ypixel = out_of_grid;
			}
			break;
		    case 1: case 2:
			xpixel = static_cast<int>(std::ceil(xpos / pixelsize));
			ypixel = static_cast<int>(2 * std::floor(ypos / (3 * side)) + 1);	
			break;
		    case 3:
			double offset = side * (3 * std::floor(posy / (3 * side)) + 2)+ (std::sqrt(3) / 3) * (x_gridsize - minor_radius);
			if(posy > ((-std::sqrt(3) / 3) * posyx + offset)){
			    xpixel = out_of_grid;
			    ypixel = out_of_grid;
			}else if(posy < ((-std::sqrt(3) / 3) * posyx + offset)){
			    xpixel = static_cast<int>(std::ceil(posx / pixelsize));
			    ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
			}
			break;
		    case 4: case 5:
			xpixel = out_of_grid;
			ypixel = out_of_grid;	
			break;
                }							 
            }   				   
	    //considering the edge of the grid along y (can this be ingrained in the above code?);
	    if(posy < (side / 2)){
		switch(x_check % 2){
		    double offset = (std::sqrt(3) / 3) * (pixelsize * (std::floor(posx / pixelsize)) + minor_radius);
		    case 0:
			if(posy > ((std::sqrt(3) / 3) * posx + offset)){
			    xpixel = static_cast<int>(std::ceil(posx / pixelsize));
			    ypixel = 1;
			}else if(posy < ((std::sqrt(3) / 3) * posx + offset)){
			    xpixel = out_of_grid;
			    ypixel = out_of_grid;
			}
			break;
		    case 1:
			offset = -offset;
			if(posy > ((std::sqrt(3) / 3) * posx + offset)){
			    xpixel = static_cast<int>(std::ceil(posx / pixelsize));
			    ypixel = 1;
			}else if(posy < ((std::sqrt(3) / 3) * posx + offset)){
			    xpixel = out_of_grid;
			    ypixel = out_of_grid;
			}
			break;
		}
	    }else if(posy > (y_gridsize - (side / 2))){
	        switch(x_check % 2){
		    double offset = y_gridsize - ((std::sqrt(3) / 3) * (minor_radius * (2* std:floor(posx / pixelsize) + 1)));    
		    case 0:
			if(posy > ((std::sqrt(3) / 3) * (posx) + offset)){
			    xpixel = out_of_grid;
			    ypixel = out_of_grid;
			}else if(posy < ((std::sqrt(3) / 3) * (posx) + offset)){
			    xpixel = static_cast<int>(std::ceil(posx / pixelsize));
			    ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
			}
			break;
		    case 1:
			offset = -offset;		    
			if(posy > ((-std::sqrt(3) / 3) * posx + offset)){
			    xpixel = out_of_grid;
			    ypixel = out_of_grid;
			}else if(posy < ((-std::sqrt(3) / 3) * posx + offset)){
			    xpixel = static_cast<int>(std::ceil(posx / pixelsize));
			    ypixel = static_cast<int>(2 * std::floor(posy / (3 * side)) + 1);
			}
			break;
		}
	    }
            return ROOT::Math::XYVector(xpixel, ypixel);
	}
	
	ROOT::Math::XYZVector getGridSize() const override {
	    double pixelx_num = getNPixels().x() * getPixelSize().x();
	    double pixely_num = getNPixels.y();
	    if((pixely_num % 2) == 1){
		y_dimension = ((pixely_num - 1) / 2) * (3 * side) + (2 * side);
	    }else if(pixely_num % 2 == 0){
		y_dimension = ((pixely_num - 1) / 2) * (3 * side) + (side / 2)

	    }
	    return{pixelx_num, pixely_num, 0}

	} 
    };
}
#endif /* ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H */
