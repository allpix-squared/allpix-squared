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

FIND_PACKAGE(LATEX)

IF(LATEX_COMPILER)
    MESSAGE(STATUS "Found LaTeX compiler.")
    SET(LATEX_COMPILER_FLAGS "-shell-escape" "-interaction=nonstopmode" CACHE STRING "Flags passed to latex.")
    INCLUDE(${CMAKE_SOURCE_DIR}/cmake/LATEX.cmake)

    SET(LATEX_OUTPUT_PATH ${CMAKE_BINARY_DIR})
    ADD_LATEX_DOCUMENT(
        usermanual/allpix-manual.tex 
        BIBFILES 
        usermanual/references.bib
        USE_BIBLATEX
        INPUTS
        usermanual/chapters/introduction.tex
        usermanual/chapters/installation.tex
        usermanual/chapters/framework.tex
        usermanual/chapters/usage.tex
        usermanual/chapters/development.tex
        usermanual/chapters/faq.tex
        usermanual/chapters/additional.tex
        usermanual/chapters/summary.tex
        usermanual/chapters/acknowledgements.tex
        usermanual/appendices/design_notes.tex
        usermanual/config.tex
        CONFIGURE
        usermanual/config.tex
        )
ELSE(LATEX_COMPILER)
    MESSAGE(STATUS "No LaTeX found, cannot compile user manual.")
ENDIF(LATEX_COMPILER)
