#ALLPIX_ENABLE_DEFAULT(OFF)

# Define module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# append pqxx flags to gcc
#set(PQXX /usr/local/include/pqxx)
#find_library(PQXX_LIB pqxx)
#find_library(PQ_LIB pq)

#find_package(PQXX)
#target_link_libraries(${TARGET_NAME} pqxx)

FIND_PACKAGE(PostgreSQL REQUIRED)
message( ${PostgreSQL} )
#set(PostgreSQL_FOUND  ${POSTGRESQL_FOUND})
#TARGET_LINK_LIBRARIES(${MODULE_NAME}  ${PostgreSQL_LIBRARIES})
#TARGET_LINK_LIBRARIES(${MODULE_NAME}  ${PQXX_LIB} ${PQ_LIB})

# Add source files to library
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    DatabaseWriterModule.cpp
)

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
