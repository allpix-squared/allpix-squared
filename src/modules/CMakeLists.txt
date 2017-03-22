###################
# Allpix2 Modules #
###################

# Option to build all modules
OPTION(BUILD_allmodules "Build all modules?" OFF)

##############################
# Build the required modules #
##############################

# example module
ALLPIX_BUILD_MODULE(example)

# geometry
ALLPIX_BUILD_MODULE(geometry_test)
ALLPIX_BUILD_MODULE(geometry_tgeo)

# deposition
ALLPIX_BUILD_MODULE(deposition_simple)

# output writers
ALLPIX_BUILD_MODULE(deposit_reader_test)
ALLPIX_BUILD_MODULE(detector_histogrammer_test)

# visualization
ALLPIX_BUILD_MODULE(visualization_test)
