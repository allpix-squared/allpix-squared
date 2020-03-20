ALLPIX_ENABLE_DEFAULT(OFF)

# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# append pqxx flags to gcc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpqxx -lpq")
find_library(PQXX_LIB pqxx)
find_library(PQ_LIB pq)
TARGET_LINK_LIBRARIES(${MODULE_NAME}  ${PQXX_LIB} ${PQ_LIB})

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    DatabaseWriterModule.cpp
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
