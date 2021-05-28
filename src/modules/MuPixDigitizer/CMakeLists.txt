# Define module
ALLPIX_DETECTOR_MODULE(MODULE_NAME)

# Add source files to module
ALLPIX_MODULE_SOURCES(
    ${MODULE_NAME} 
    MuPixDigitizerModule.cpp
    MuPixModel.cpp
    Models/MuPix10.cpp
    Models/MuPix10Double.cpp
    Models/MuPix10Ramp.cpp
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
