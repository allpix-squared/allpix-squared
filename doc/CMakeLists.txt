#############################################
# Doxygen target to generate API reference  #
#############################################

FIND_PACKAGE(Doxygen)

IF(DOXYGEN_FOUND)
  CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/doc/reference/Doxyfile.in ${CMAKE_BINARY_DIR}/doc/reference/Doxyfile @ONLY)
  ADD_CUSTOM_TARGET(reference
    ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doc/reference/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc/reference
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
ENDIF(DOXYGEN_FOUND)

###############################################
# LaTeX target to compile the PDF user manual #
###############################################

# Information and the manual on the UseLATEX.cmake package can be found here
# http://www.cmake.org/Wiki/CMakeUserUseLATEX
# The code resides here:
# https://github.com/kmorel/UseLATEX

FIND_PACKAGE(LATEX)

IF(LATEX_COMPILER)
  MESSAGE(STATUS "Found LaTeX compiler.")
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/UseLATEX.cmake)
  SET(LATEX_OUTPUT_PATH ${CMAKE_BINARY_DIR})
  ADD_LATEX_DOCUMENT(usermanual/allpix-manual.tex 
    INPUTS
    usermanual/symbols.tex
    usermanual/references.tex
    NO_DEFAULT
    )
ELSE(LATEX_COMPILER)
  MESSAGE(STATUS "No LaTeX found, cannot compile user manual.")
ENDIF(LATEX_COMPILER)
