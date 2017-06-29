##############
# RCE Writer #
##############

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
    RCEWriterModule.cpp
    # ADD SOURCE FILES HERE...
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
