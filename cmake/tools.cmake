# Retrieve the allpix version string from git describe
FUNCTION(get_version PROJECT_VERSION)
    # Check if this is a source tarball build
    IF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
        SET(SOURCE_PACKAGE 1)
    ENDIF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

    # Set package version
    IF(NOT SOURCE_PACKAGE)
        SET(TAG_FOUND FALSE)

        # Get the version from last git tag plus number of additional commits:
        FIND_PACKAGE(Git QUIET)
        IF(GIT_FOUND)
            EXEC_PROGRAM(git ${CMAKE_CURRENT_SOURCE_DIR} ARGS describe --tags HEAD OUTPUT_VARIABLE ${PROJECT_VERSION})
            STRING(REGEX REPLACE "^release-" "" ${PROJECT_VERSION} ${${PROJECT_VERSION}})
            STRING(REGEX REPLACE "([v0-9.]+)-([0-9]+)-([A-Za-z0-9]+)" "\\1+\\2^\\3" ${PROJECT_VERSION} ${${PROJECT_VERSION}})
            EXEC_PROGRAM(git ARGS status --porcelain ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE PROJECT_STATUS)
            IF(PROJECT_STATUS STREQUAL "")
                MESSAGE(STATUS "Git project directory is clean.")
            ELSE(PROJECT_STATUS STREQUAL "")
                MESSAGE(STATUS "Git project directory is dirty:\n ${PROJECT_STATUS}.")
            ENDIF(PROJECT_STATUS STREQUAL "")

            EXEC_PROGRAM(git ${CMAKE_CURRENT_SOURCE_DIR} ARGS describe --exact-match --tags HEAD OUTPUT_VARIABLE GIT_TAG RETURN_VALUE GIT_RETURN)
            IF(GIT_RETURN EQUAL 0)
                SET(TAG_FOUND TRUE)
            ENDIF()
        ELSE(GIT_FOUND)
            MESSAGE(STATUS "Git repository present, but could not find git executable.")
        ENDIF(GIT_FOUND)
    ELSE(NOT SOURCE_PACKAGE)
        # If we don't have git we can not really do anything
        MESSAGE(STATUS "Source tarball build - no repository present.")
        SET(TAG_FOUND TRUE)
    ENDIF(NOT SOURCE_PACKAGE)

    # Set the project version in the parent scope
    SET(TAG_FOUND ${TAG_FOUND} PARENT_SCOPE)

    # Set the project version in the parent scope
    SET(${PROJECT_VERSION} ${${PROJECT_VERSION}} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(add_runtime_dep RUNTIME_NAME)
    GET_FILENAME_COMPONENT(THISPROG ${RUNTIME_NAME} PROGRAM)
    LIST(APPEND ALLPIX_RUNTIME_DEPS ${THISPROG})
    LIST(REMOVE_DUPLICATES ALLPIX_RUNTIME_DEPS)
    SET(ALLPIX_RUNTIME_DEPS ${ALLPIX_RUNTIME_DEPS} CACHE INTERNAL "ALLPIX_RUNTIME_DEPS")
ENDFUNCTION()

FUNCTION(add_runtime_lib RUNTIME_NAME)
    FOREACH(name ${RUNTIME_NAME})
        GET_FILENAME_COMPONENT(THISLIB ${name} DIRECTORY)
        LIST(APPEND ALLPIX_RUNTIME_LIBS ${THISLIB})
        LIST(REMOVE_DUPLICATES ALLPIX_RUNTIME_LIBS)
        SET(ALLPIX_RUNTIME_LIBS ${ALLPIX_RUNTIME_LIBS} CACHE INTERNAL "ALLPIX_RUNTIME_LIBS")
    ENDFOREACH()
ENDFUNCTION()
