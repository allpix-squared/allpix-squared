# This module tries to find the lcio installation on your system.
# Once done this will define
#
#  LCIO_FOUND - system has lcio
#  LCIO_INCLUDE_DIR - ~ the lcio include directory
#  LCIO_LIBRARY - Link these to use lcio

FIND_PATH(LCIO_INCLUDE_DIR
  NAMES   lcio.h
  PATHS   /usr/local/include
  /usr/include
  /usr/include/lcio
  /usr/local/include/lcio
  /opt/local/include
  /sw/include
  $ENV{LCIO}/include
  )


SET(LCIO_LIBNAME liblcio.so)

FIND_LIBRARY(LCIO_LIBRARY
  NAMES ${LCIO_LIBNAME}
  PATHS /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
  $ENV{LCIO}/lib
  )

IF (LCIO_LIBRARY)
  IF(LCIO_INCLUDE_DIR)
    SET(LCIO_FOUND TRUE)
    MESSAGE(STATUS "Found lcio: ${LCIO_INCLUDE_DIR}, ${LCIO_LIBRARY}")
  ELSE(LCIO_INCLUDE_DIR)
    SET(LCIO_FOUND FALSE)
    MESSAGE(STATUS "lcio headers NOT FOUND. Please refer to the documentation for instructions.")
  ENDIF(LCIO_INCLUDE_DIR)
ELSE (LCIO_LIBRARY)
  SET(LCIO_FOUND FALSE)
  MESSAGE(STATUS "liblcio NOT FOUND.")
ENDIF (LCIO_LIBRARY)

