########################
# Allpix2 Dummy Module #
########################

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

FIND_PACKAGE(ROOT REQUIRED COMPONENTS TreePlayer)
IF(NOT ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Module needs TreePlayer library")
ENDIF(NOT ROOT_FOUND)

# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
    ROOTObjectReaderModule.cpp
    # ADD SOURCE FILES HERE...
)
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${ROOT_LIBRARIES})

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
