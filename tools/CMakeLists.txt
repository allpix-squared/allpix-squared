# Include set of separate tools shipped with the framework
OPTION(BUILD_TOOLS "Build additional tools and executables" ON)

if(BUILD_TOOLS)
    # Build the MeshConverter
    ADD_SUBDIRECTORY(mesh_converter)

    # Install the ROOT helper macro's for analysis
    ADD_SUBDIRECTORY(root_analysis_macros)

    # Add APF filed format helper tools
    ADD_SUBDIRECTORY(apf_tools)

    # Add APF filed format helper tools
    ADD_SUBDIRECTORY(weightingpotential_generator)
ENDIF()
