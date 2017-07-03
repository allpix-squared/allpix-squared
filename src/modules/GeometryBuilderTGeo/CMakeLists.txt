# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Print message if GDML support is not enabled in ROOT
IF(ROOT_gdml_FOUND)
  message(STATUS "Geometry export in GDML format is enabled.")
  ADD_DEFINITIONS(-DROOT_GDML)
ELSE()
  message(STATUS "Geometry export in GDML format is disabled. To enable it, configure and compile ROOT with the option -Dgdml=ON.")
ENDIF()


# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
  GeometryBuilderTGeoModule.cpp
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
