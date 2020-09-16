# Find PostGreSQL C++ library and header file
# Sets
#   PQXX_FOUND                 to 0 or 1 depending on result
#   PQXX_INCLUDE_DIRECTORIES  to the directory containing mysql.h
#   PQXX_LIBRARIES            to the MySQL client library (and any dependents required)
# If PQXX_REQUIRED is defined, then a fatal error message will be generated if libpqxx is not found
IF(NOT PQXX_INCLUDE_DIRECTORIES OR NOT PQXX_LIBRARIES)

    FIND_PACKAGE(PostgreSQL REQUIRED)
    IF(PostgreSQL_FOUND)
        FILE(TO_CMAKE_PATH "$ENV{PQXX_DIR}" _PQXX_DIR)

        FIND_LIBRARY(PQXX_LIBRARY
            NAMES libpqxx pqxx
            PATHS
            ${_PQXX_DIR}/lib
            ${_PQXX_DIR}
            ${CMAKE_INSTALL_PREFIX}/lib
            ${CMAKE_INSTALL_PREFIX}/bin
            /usr/local/pgsql/lib
            /usr/local/lib
            /usr/lib
            /usr/lib/x86_64-linux-gnu/
            DOC "Location of libpqxx library"
            NO_DEFAULT_PATH
        )

        FIND_PATH(PQXX_HEADER_PATH
            NAMES pqxx/pqxx
            PATHS
            ${_PQXX_DIR}/include
            ${_PQXX_DIR}
            ${CMAKE_INSTALL_PREFIX}/include
            /usr/local/pgsql/include
            /usr/local/include
            /usr/include
            DOC "Path to pqxx/pqxx header file. Do not include the 'pqxx' directory in this value."
            NO_DEFAULT_PATH
        )
    ENDIF()

    IF(PQXX_HEADER_PATH AND PQXX_LIBRARY)

        SET(PQXX_FOUND 1 CACHE INTERNAL "PQXX found" FORCE)
        SET(PQXX_INCLUDE_DIRECTORIES "${PQXX_HEADER_PATH};${PostgreSQL_INCLUDE_DIRECTORIES}" CACHE STRING "Include directories for PostGreSQL C++ library"  FORCE)
        SET(PQXX_LIBRARIES "${PQXX_LIBRARY};${PostgreSQL_LIBRARIES}" CACHE STRING "Link libraries for PostGreSQL C++ interface" FORCE)

        MARK_AS_ADVANCED(FORCE PQXX_INCLUDE_DIRECTORIES)
        MARK_AS_ADVANCED(FORCE PQXX_LIBRARIES)
    ELSE()
        MESSAGE("PQXX NOT FOUND")
    ENDIF()
ENDIF()
