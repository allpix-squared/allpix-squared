# Add mesh_plotter executable
ADD_EXECUTABLE(mesh_plotter mesh_plotter.cpp)

# Link libraries
TARGET_LINK_LIBRARIES(mesh_plotter ${ALLPIX_DEPS_LIBRARIES})
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

INSTALL(TARGETS mesh_plotter 
    RUNTIME DESTINATION bin/tcad_dfise_converter)
