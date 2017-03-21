###################
# Allpix2 Modules #
###################

# For every module, build a separate library to be loaded by allpix core

MACRO(option_build_module dir)
  OPTION(BUILD_${dir} "Build module in directory ${dir}?" ON)

  IF(BUILD_${dir} OR BUILD_allmodules)
    MESSAGE( STATUS "Building module: " ${dir} )
    ADD_SUBDIRECTORY(${dir})
  ENDIF()
ENDMACRO()

# Option to build all modules
OPTION(BUILD_allmodules "Build all modules?" OFF)


##############################
# Build the required modules #
##############################

# example module
option_build_module(example)

# geometry
option_build_module(geometry_test)
option_build_module(geometry_tgeo)

# deposition
option_build_module(deposition_simple)

# output writers
option_build_module(deposit_reader_test)
option_build_module(detector_histogrammer_test)

# visualization
option_build_module(visualization_test)
