#############################################
# Doxygen target to generate API reference  #
#############################################

FIND_PACKAGE(Doxygen)

IF(DOXYGEN_FOUND)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/doc/reference/Doxyfile.in ${CMAKE_BINARY_DIR}/reference/Doxyfile @ONLY)
    ADD_CUSTOM_TARGET(reference
        ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/reference/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/reference
        COMMENT "Generating API documentation with Doxygen" 
        VERBATIM
        )
ENDIF(DOXYGEN_FOUND)

###############################################
# LaTeX target to compile the PDF user manual #
###############################################

# Information and the manual on the UseLATEX.cmake package can be found here
# http://www.cmake.org/Wiki/CMakeUserUseLATEX

FIND_PACKAGE(LATEX)

# Only add target if we have LaTeX
IF(LATEX_COMPILER)
    MESSAGE(STATUS "Found LaTeX compiler.")
    SET(LATEX_COMPILER_FLAGS "-shell-escape" "-interaction nonstopmode" "-halt-on-error" "-file-line-error" CACHE STRING "Flags passed to latex.")
    INCLUDE(${CMAKE_SOURCE_DIR}/cmake/LATEX.cmake)
    
    # Check for pandoc to add module documentation
    INCLUDE(${CMAKE_SOURCE_DIR}/cmake/PANDOC.cmake)
    IF(PANDOC_EXECUTABLE)
        # Loop through all modules and fetch their README
        SET(module_dir ${CMAKE_SOURCE_DIR}/src/modules)
        FILE(GLOB subdirs RELATIVE ${module_dir} ${module_dir}/*)
        FOREACH(subdir ${subdirs})
            IF(EXISTS ${module_dir}/${subdir}/README.md AND NOT ${subdir} STREQUAL "Dummy")
                add_document(
                    ${CMAKE_BINARY_DIR}/usermanual/modules/${subdir}.tex
                    SOURCES               ${module_dir}/${subdir}/README.md
                    PRODUCT_DIRECTORY    /usermanual/modules/
                    PANDOC_DIRECTIVES    -t latex --listings
                    NO_EXPORT_PRODUCT
                )
                SET(module_tex_files ${module_tex_files} ${CMAKE_BINARY_DIR}/usermanual/modules/${subdir}.tex)
            ENDIF()
        ENDFOREACH()
    ELSE()
        SET(module_tex_files)
        MESSAGE(WARNING "No pandoc command, cannot add module documentation to user manual")
    ENDIF()
    
    # Build array of LaTex files to add to the documentation
    FOREACH(module_tex_file ${module_tex_files})
        IF(NOT ALLPIX_MODULE_FILES)
            SET(ALLPIX_MODULE_FILES "{${module_tex_file}")
        ELSE()
            SET(ALLPIX_MODULE_FILES "${ALLPIX_MODULE_FILES},${module_tex_file}")
        ENDIF()
    ENDFOREACH()
    IF(ALLPIX_MODULE_FILES)
        SET(ALLPIX_MODULE_FILES "${ALLPIX_MODULE_FILES}}")
    ELSE()
        SET(ALLPIX_MODULE_FILES "")
    ENDIF()
                
    # Check if we have biber and do not build if not
    IF(BIBER_COMPILER)
        SET(LATEX_OUTPUT_PATH ${CMAKE_BINARY_DIR}/usermanual)
        ADD_LATEX_DOCUMENT(
            usermanual/allpix-manual.tex 
            BIBFILES 
            usermanual/references.bib
            USE_BIBLATEX
            IMAGES
            logo.png
            INPUTS
            usermanual/chapters/introduction.tex
            usermanual/chapters/quick_start.tex
            usermanual/chapters/installation.tex
            usermanual/chapters/framework.tex
            usermanual/chapters/getting_started.tex
            usermanual/chapters/development.tex
            usermanual/chapters/faq.tex
            usermanual/chapters/additional.tex
            usermanual/chapters/acknowledgements.tex
            usermanual/appendices/design_notes.tex
            usermanual/appendices/example_output.tex
            usermanual/config.tex
            CONFIGURE
            usermanual/config.tex
            EXCLUDE_FROM_ALL
            DEPENDS ${module_tex_files})
    ELSE()
        MESSAGE(WARNING "No biber command, cannot compile user manual.")
    ENDIF()
ELSE(LATEX_COMPILER)
    MESSAGE(WARNING "No LaTeX found, cannot compile user manual.")
ENDIF(LATEX_COMPILER)
