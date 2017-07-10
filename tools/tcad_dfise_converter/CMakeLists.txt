# Add TCAD dfise converter executable 
ADD_EXECUTABLE(tcad_dfise_converter 
    dfise_converter.cpp
    read_dfise.cpp
)

# Create install target
INSTALL(TARGETS tcad_dfise_converter
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
