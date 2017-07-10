# Add TCAD dfise converter executable 
ADD_EXECUTABLE(tcad_dfise_converter 
    dfise_converter.cpp
    read_dfise.cpp
)

# Link the dependency libraries
TARGET_LINK_LIBRARIES(tcad_dfise_converter ${ALLPIX_DEPS_LIBRARIES})

# Create install target
INSTALL(TARGETS tcad_dfise_converter
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
