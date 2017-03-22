########################
# Allpix2 Dummy Module #
########################

# Define module
ALLPIX_MODULE(MODULE_NAME)

# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
    DummyModule.cpp
    # ADD SOURCE FILES HERE...
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
