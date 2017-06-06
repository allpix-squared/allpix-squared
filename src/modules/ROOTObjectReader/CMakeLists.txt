########################
# Allpix2 Dummy Module #
########################

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
    ROOTObjectReaderModule.cpp
)
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${ROOT_LIBRARIES})

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
