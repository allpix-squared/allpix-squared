# Include set of separate tools shipped with the framework

# Build the TCAD converter
ADD_SUBDIRECTORY(tcad_dfise_converter)

# Install the ROOT helper macro's for analysis
ADD_SUBDIRECTORY(root_analysis_macros)

# Add APF filed format helper tools
ADD_SUBDIRECTORY(apf_tools)

# Add APF filed format helper tools
ADD_SUBDIRECTORY(weightingpotential_generator)
