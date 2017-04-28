###################
# Allpix2 Modules #
###################

# Option to build all modules
OPTION(BUILD_ALL_MODULES "Build all modules?" OFF)

# set the models path
ADD_DEFINITIONS(-DALLPIX_MODEL_DIRECTORY="${MODEL_DIRECTORY}" -DALLPIX_MODEL_SUFFIX="${MODEL_SUFFIX}")

# reset the saved libraries
SET(ALLPIX_MODULE_LIBRARIES "" CACHE INTERNAL "Module libraries")

##############################
# Build the common logic     #
##############################

# FIXME: THIS IS TEMPORARY AND SHOULD LATER BE FIXED IN A BETTER WAY
ADD_SUBDIRECTORY(common)
SET(ALLPIX_DEPS_LIBRARIES ${ALLPIX_DEPS_LIBRARIES} AllPixModulesCommon)

##############################
# Build the required modules #
##############################

# example module
ALLPIX_BUILD_MODULE(Example)

# geometry
ALLPIX_BUILD_MODULE(GeometryBuilderTGeo)
ALLPIX_BUILD_MODULE(GeometryBuilderGeant4)

# input readers
ALLPIX_BUILD_MODULE(ElectricFieldReaderInit)

# deposition
ALLPIX_BUILD_MODULE(DepositionGeant4)

# propagation
ALLPIX_BUILD_MODULE(SimplePropagation)

# transfer
ALLPIX_BUILD_MODULE(SimpleTransfer)

# visualization
ALLPIX_BUILD_MODULE(VisualizationGeant4)

# TEST output writers
ALLPIX_BUILD_MODULE(deposit_reader_test)
ALLPIX_BUILD_MODULE(detector_histogrammer_test)
