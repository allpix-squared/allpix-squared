###################
# Allpix2 Modules #
###################

# Option to build all modules
OPTION(BUILD_allmodules "Build all modules?" OFF)

##############################
# Build the required modules #
##############################

# example module
allpix_build_module(example)

# geometry
allpix_build_module(geometry_test)
allpix_build_module(geometry_tgeo)

# deposition
allpix_build_module(deposition_simple)

# output writers
allpix_build_module(deposit_reader_test)
allpix_build_module(detector_histogrammer_test)

# visualization
allpix_build_module(visualization_test)
