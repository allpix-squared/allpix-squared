##############
# RCE Writer #
##############

# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Add library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    RCEWriterModule.cpp
)

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Tree)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
