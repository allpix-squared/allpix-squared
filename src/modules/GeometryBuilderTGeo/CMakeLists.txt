################################################
# Geometry Builder Module, TGeo Implementation #
################################################

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME "TGeoBuilderModule")

# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    TGeoBuilderModule.cpp
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
