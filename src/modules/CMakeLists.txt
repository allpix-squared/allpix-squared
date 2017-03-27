###################
# Allpix2 Modules #
###################

# Option to build all modules
OPTION(BUILD_allmodules "Build all modules?" OFF)

##############################
# Build the common logic     #
##############################

# FIXME: THIS IS TEMPORARY AND SHOULD LATER BE FIXED IN A BETTER WAY
add_subdirectory(common)
set(ALLPIX_DEPS_LIBRARIES ${ALLPIX_DEPS_LIBRARIES} AllPixModulesCommon)

##############################
# Build the required modules #
##############################

# example module
ALLPIX_BUILD_MODULE(Example)

# geometry
ALLPIX_BUILD_MODULE(GeometryBuilderTGeo)
ALLPIX_BUILD_MODULE(GeometryBuilderGeant4)

# deposition
ALLPIX_BUILD_MODULE(DepositionGeant4)

# visualization
ALLPIX_BUILD_MODULE(VisualizationGeant4)

# TEST output writers
ALLPIX_BUILD_MODULE(deposit_reader_test)
ALLPIX_BUILD_MODULE(detector_histogrammer_test)
