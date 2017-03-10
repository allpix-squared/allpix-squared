FUNCTION(GetVersionString)
  # Check if this is a source tarball build
  IF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
    SET(SOURCE_PACKAGE 1)
  ENDIF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

  # Set package version
  IF(NOT SOURCE_PACKAGE)
    # Get the version from last git tag plus number of additional commits:
    FIND_PACKAGE(Git QUIET)
    IF(GIT_FOUND)
      EXEC_PROGRAM(git ${CMAKE_CURRENT_SOURCE_DIR} ARGS describe --tags HEAD OUTPUT_VARIABLE ALLPIX_LIB_VERSION)
      STRING(REGEX REPLACE "^release-" "" ALLPIX_LIB_VERSION ${ALLPIX_LIB_VERSION})
      STRING(REGEX REPLACE "([v0-9.]+)-([0-9]+)-([A-Za-z0-9]+)" "\\1+\\2~\\3" ALLPIX_LIB_VERSION ${ALLPIX_LIB_VERSION})
      EXEC_PROGRAM(git ARGS status --porcelain ${CMAKE_CURRENT_SOURCE_DIR}/core OUTPUT_VARIABLE ALLPIX_CORE_STATUS)
      IF(ALLPIX_CORE_STATUS STREQUAL "")
    MESSAGE("-- Git: corelib directory is clean.")
      ELSE(ALLPIX_CORE_STATUS STREQUAL "")
    MESSAGE("-- Git: corelib directory is dirty:\n ${ALLPIX_CORE_STATUS}.")
      ENDIF(ALLPIX_CORE_STATUS STREQUAL "")
    ELSE(GIT_FOUND)
      SET(ALLPIX_LIB_VERSION ${ALLPIX_VERSION})
      MESSAGE("-- Git repository present, but could not find git executable.")
    ENDIF(GIT_FOUND)
  ELSE(NOT SOURCE_PACKAGE)
    # If we don't have git we take the hard-set version.
    SET(ALLPIX_LIB_VERSION ${ALLPIX_VERSION})
    MESSAGE("-- Source tarball build - no repository present.")
  ENDIF(NOT SOURCE_PACKAGE)
  MESSAGE("-- Determined allpix corelib API version ${ALLPIX_LIB_VERSION}.")

  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.h" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
  INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")
ENDFUNCTION(GetVersionString)

