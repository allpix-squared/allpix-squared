#############################################
# Doxygen target to generate API reference  #
#############################################

FIND_PACKAGE(Doxygen)

IF(DOXYGEN_FOUND)
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/doc/reference/Doxyfile.in ${CMAKE_BINARY_DIR}/reference/Doxyfile @ONLY)
    ADD_CUSTOM_TARGET(
        ${CMAKE_PROJECT_NAME}-reference
        ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/reference/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/reference
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
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
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/LATEX.cmake)

    SET(HTLATEX_COMPILER_TEX4HT_FLAGS "usermanual/html/allpix,charset=utf-8" CACHE STRING "Flags passed to Tex4HT" FORCE)
    SET(HTLATEX_COMPILER_TEX4HT_POSTPROCESSOR_FLAGS "-utf8 -cunihtf" CACHE STRING "Flags passed to the postprocessor of Tex4HT" FORCE)

    # Markdown files that should be included in the manual relative to the root of the repository
    SET(DOC_README_FILES
            tools/mesh_converter/README.md
            tools/root_analysis_macros/README.md
    )

    # Check for pandoc for markdown conversion
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/PANDOC.cmake)
    IF(PANDOC_EXECUTABLE)
        # Loop through all modules and fetch their README
        SET(module_dir ${PROJECT_SOURCE_DIR}/src/modules)
        FILE(GLOB subdirs RELATIVE ${module_dir} ${module_dir}/*)
        FOREACH(subdir ${subdirs})
            IF(EXISTS ${module_dir}/${subdir}/README.md AND NOT ${subdir} STREQUAL "Dummy")
                ADD_PANDOC_DOCUMENT(
                    ${CMAKE_BINARY_DIR}/usermanual/modules/${subdir}.tex
                    SOURCES              ${module_dir}/${subdir}/README.md
                    PRODUCT_DIRECTORY    usermanual/modules/
                    PANDOC_DIRECTIVES    -t latex --listings --biblatex
                    NO_EXPORT_PRODUCT
                )
                SET(module_tex_files ${module_tex_files} ${CMAKE_BINARY_DIR}/usermanual/modules/${subdir}.tex)
            ENDIF()
        ENDFOREACH()

        # Loop through all modules and fetch their README
        SET(example_dir ${PROJECT_SOURCE_DIR}/examples)
        FILE(GLOB subdirs RELATIVE ${example_dir} ${example_dir}/*)
        FOREACH(subdir ${subdirs})
            IF(EXISTS ${example_dir}/${subdir}/README.md)
                ADD_PANDOC_DOCUMENT(
                    ${CMAKE_BINARY_DIR}/usermanual/examples/${subdir}.tex
                    SOURCES              ${example_dir}/${subdir}/README.md
                    PRODUCT_DIRECTORY    usermanual/examples/
                    PANDOC_DIRECTIVES    -t latex --listings --biblatex
                    NO_EXPORT_PRODUCT
                )
                SET(example_tex_files ${example_tex_files} ${CMAKE_BINARY_DIR}/usermanual/examples/${subdir}.tex)
            ENDIF()
        ENDFOREACH()

        # Generate other tex files from README we need and put them in the MD folder of the user manual
        SET(other_tex_files "")
        FOREACH(doc_readme_file ${DOC_README_FILES})
            GET_FILENAME_COMPONENT(readme_full_dir ${doc_readme_file} DIRECTORY)
            GET_FILENAME_COMPONENT(readme_parent_dir ${readme_full_dir} DIRECTORY)
            GET_FILENAME_COMPONENT(readme_dir_name ${readme_full_dir} NAME)

            ADD_PANDOC_DOCUMENT(
                ${CMAKE_BINARY_DIR}/usermanual/md/${readme_parent_dir}/${readme_dir_name}.tex
                SOURCES
                ${PROJECT_SOURCE_DIR}/${doc_readme_file}
                PRODUCT_DIRECTORY
                usermanual/md/${readme_parent_dir}/
                PANDOC_DIRECTIVES
                -t
                latex
                --listings
                --biblatex
                NO_EXPORT_PRODUCT)

            SET(_other_tex_files ${_other_tex_files}
                                 ${CMAKE_BINARY_DIR}/usermanual/md/${readme_parent_dir}/${readme_dir_name}.tex)
        ENDFOREACH()
    ELSE()
        SET(_module_tex_files)
        SET(_example_tex_files)
        SET(_other_tex_files "")
        MESSAGE(WARNING "No pandoc command, cannot add Markdown documentation to user manual")
    ENDIF()

    # Build array of LaTex files to add to the documentation
    LIST(LENGTH _module_tex_files NUM_MODULES)
    MESSAGE(STATUS "Manual: adding ${NUM_MODULES} module descriptions")
    FOREACH(module_tex_file ${_module_tex_files})
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

    LIST(LENGTH _example_tex_files NUM_EXAMPLES)
    MESSAGE(STATUS "Manual: adding ${NUM_EXAMPLES} example descriptions")
    FOREACH(example_tex_file ${_example_tex_files})
        IF(NOT ALLPIX_EXAMPLE_FILES)
            SET(ALLPIX_EXAMPLE_FILES "{${example_tex_file}")
        ELSE()
            SET(ALLPIX_EXAMPLE_FILES "${ALLPIX_EXAMPLE_FILES},${example_tex_file}")
        ENDIF()
    ENDFOREACH()
    IF(ALLPIX_EXAMPLE_FILES)
        SET(ALLPIX_EXAMPLE_FILES "${ALLPIX_EXAMPLE_FILES}}")
    ELSE()
        SET(ALLPIX_EXAMPLE_FILES "")
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
            cc-by.png
            usermanual/figures/telescope.png
            usermanual/figures/passive_materials.png
            usermanual/figures/ci.png
            usermanual/figures/linegraph_hrcmos_collected.png
            INPUTS
            usermanual/chapters/introduction.tex
            usermanual/chapters/quick_start.tex
            usermanual/chapters/installation.tex
            usermanual/chapters/testing.tex
            usermanual/chapters/framework.tex
            usermanual/chapters/getting_started.tex
            usermanual/chapters/objects.tex
            usermanual/chapters/modules.tex
            usermanual/chapters/examples.tex
            usermanual/chapters/development.tex
            usermanual/chapters/faq.tex
            usermanual/chapters/additional.tex
            usermanual/chapters/acknowledgements.tex
            usermanual/config.tex
            usermanual/tikz/transformations.tex
            usermanual/html/allpix.cfg
            usermanual/html/tools.sty
            usermanual/html/js/fw.js
            usermanual/html/js/sidebar.js
            usermanual/html/css/custom.css
            CONFIGURE
            usermanual/config.tex
            EXCLUDE_FROM_ALL
            DEPENDS ${module_tex_files} ${example_tex_files} ${other_tex_files})
    ELSE()
        MESSAGE(WARNING "No biber command, cannot compile user manual.")
    ENDIF()
ELSE(LATEX_COMPILER)
    MESSAGE(WARNING "No LaTeX found, cannot compile user manual.")
ENDIF(LATEX_COMPILER)
