####################################
# Module receiving charge deposits #
####################################

# Define module and return the generated name as MODULE_NAME
ALLPIX_MODULE(MODULE_NAME)

# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
    TestDepositReaderModule.cpp
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
