# Option to build all modules
OPTION(BUILD_ALL_MODULES "Build all modules?" OFF)

# reset the saved libraries
SET(ALLPIX_MODULE_LIBRARIES "" CACHE INTERNAL "Module libraries")

# Build the modules 

# model readers
ALLPIX_BUILD_MODULE(DefaultModelReader)

# geometry
ALLPIX_BUILD_MODULE(GeometryBuilderTGeo)
ALLPIX_BUILD_MODULE(GeometryBuilderGeant4)

# electric field readers
ALLPIX_BUILD_MODULE(ElectricFieldReaderInit)
ALLPIX_BUILD_MODULE(ElectricFieldInputLinear)

# deposition
ALLPIX_BUILD_MODULE(DepositionGeant4)

# propagation
ALLPIX_BUILD_MODULE(SimplePropagation)

# transfer
ALLPIX_BUILD_MODULE(SimpleTransfer)

# digitization
ALLPIX_BUILD_MODULE(DefaultDigitizer)

# visualization
ALLPIX_BUILD_MODULE(VisualizationGeant4)

# TEST output writers
ALLPIX_BUILD_MODULE(deposit_reader_test)
ALLPIX_BUILD_MODULE(detector_histogrammer_test)
