################################################
# Geometry Builder Module, TGeo Implementation #
################################################

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME "TGeoBuilderModule")

# Print message if GDML support is not enabled in ROOT
#GET_DIRECTORY_PROPERTY(ROOT_GDML DIRECTORY ${ROOT_DIR} DEFINITION gdml)
#FIND_PACKAGE(ROOT OPTIONAL_COMPONENTS gdml)
#IF(gdml)
#IF(ROOT_GDML)
IF(ROOT_gdml_FOUND)
  message(STATUS "Geometry export in GDML format is enabled.")
  ADD_DEFINITIONS(-DROOT_GDML)
ELSE()
  message(STATUS "Geometry export in GDML format is disabled. To enable it, configure and compile ROOT with the option -Dgdml=ON.")
ENDIF()


# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    TGeoBuilderModule.cpp
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
