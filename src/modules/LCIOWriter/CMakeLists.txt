ALLPIX_ENABLE_DEFAULT(OFF)

# LCIO library is required for LCIO file writing, obviously
FIND_PACKAGE(LCIO REQUIRED)


# Define module and return the generated name as MODULE_NAME
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

INCLUDE_DIRECTORIES(SYSTEM ${LCIO_INCLUDE_DIRS} )
TARGET_LINK_LIBRARIES( ${MODULE_NAME} ${LCIO_LIBRARIES} )

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME} 
LCIOWriterModule.cpp
	# ADD SOURCE FILES HERE...
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})

