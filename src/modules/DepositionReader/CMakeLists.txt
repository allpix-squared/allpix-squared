# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    DepositionReaderModule.cpp
)

IF(${ROOT_VERSION} VERSION_GREATER "6.10")
    TARGET_COMPILE_DEFINITIONS(${MODULE_NAME} PRIVATE ROOT_TTREEREADER_VERBOSE)
ENDIF()

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
