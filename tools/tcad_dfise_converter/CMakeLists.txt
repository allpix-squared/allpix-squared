# Add TCAD dfise converter executable
ADD_EXECUTABLE(dfise_converter
    dfise_converter.cpp
    read_dfise.cpp
    ../../src/core/utils/log.cpp
)

#Build the TCAD converter
ADD_SUBDIRECTORY(data)

# Link the dependency libraries
TARGET_LINK_LIBRARIES(dfise_converter ${ALLPIX_DEPS_LIBRARIES})

# Include Eigen dependency
FIND_PACKAGE(Eigen3 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${EIGEN3_INCLUDE_DIR})

# Create install target
INSTALL(TARGETS dfise_converter
    COMPONENT tools
    RUNTIME DESTINATION bin/tcad_dfise_converter)
