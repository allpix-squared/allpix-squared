# Add mesh_plotter executable
ADD_EXECUTABLE(mesh_plotter mesh_plotter.cpp)

# Link libraries
TARGET_LINK_LIBRARIES(mesh_plotter ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})
INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIRS})

INSTALL(TARGETS mesh_plotter
    RUNTIME DESTINATION bin/tcad_dfise_converter)
