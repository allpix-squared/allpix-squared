# SPDX-FileCopyrightText: 2016-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

FIND_PACKAGE(Python COMPONENTS Interpreter)
IF(NOT PYTHON_FOUND)
    MESSAGE(WARNING "Could not find Python interpreter, no documentation generation possible")
    RETURN()
ENDIF()

############################################
# Doxygen target to generate API reference #
############################################

SET(DOCS_DOXYGEN_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/reference")

FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
    MESSAGE(STATUS "Docs: Adding target for reference generation with Doxygen")
    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/reference/Doxyfile" "${DOCS_DOXYGEN_BINARY_DIR}/Doxyfile" @ONLY)
    ADD_CUSTOM_TARGET(
        apsq_ref_doxygen
        COMMAND Doxygen::doxygen "${DOCS_DOXYGEN_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${DOCS_DOXYGEN_BINARY_DIR}"
        COMMENT "Generating API reference with Doxygen"
        VERBATIM)
    ADD_DEPENDENCIES(apsq_ref_doxygen Doxygen::doxygen Python::Interpreter)
ENDIF()

FIND_PACKAGE(Doxybook2)
IF(DOXYBOOK2_FOUND)
    MESSAGE(STATUS "Docs: Adding target for reference markdown generation")
    SET(REF_MARKDOWN_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/reference_markdown")
    ADD_CUSTOM_TARGET(
        apsq_ref_markdown
        COMMAND
            ${CMAKE_COMMAND} -E remove_directory -f ${REF_MARKDOWN_BINARY_DIR}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory -f ${REF_MARKDOWN_BINARY_DIR}
        COMMAND
            Doxybook2::doxybook2 -q -i "${DOCS_DOXYGEN_BINARY_DIR}/xml" -o "${REF_MARKDOWN_BINARY_DIR}"
            -t "${CMAKE_CURRENT_SOURCE_DIR}/reference/doxybook"
            -c "${CMAKE_CURRENT_SOURCE_DIR}/reference/doxybook/config.json"
        DEPENDS apsq_ref_doxygen
        WORKING_DIRECTORY "${DOCS_DOXYGEN_BINARY_DIR}"
        COMMENT "Generating Markdown tree from Doxygen XML"
        VERBATIM)
    ADD_DEPENDENCIES(apsq_ref_markdown Doxybook2::doxybook2 Python::Interpreter)
ENDIF()

#######################################
# Target to create Markdown file tree #
#######################################

SET(DOCS_MARKDOWN_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/usermanual_markdown")
SET(DOCS_PYTHON_HELPER "${CMAKE_CURRENT_SOURCE_DIR}/convert/cmake_helper.py")

ADD_CUSTOM_TARGET(
    apsq_docs_markdown
    COMMAND
        ${CMAKE_COMMAND} -E remove_directory -f ${DOCS_MARKDOWN_BINARY_DIR}
    COMMAND
        Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "glob_dir"
        "${CMAKE_CURRENT_SOURCE_DIR}/usermanual" "${DOCS_MARKDOWN_BINARY_DIR}"
    COMMAND
        Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "glob_readme"
        "${PROJECT_SOURCE_DIR}/src/modules" "${DOCS_MARKDOWN_BINARY_DIR}/08_modules"
    COMMAND
        Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "glob_readme"
        "${PROJECT_SOURCE_DIR}/examples" "${DOCS_MARKDOWN_BINARY_DIR}/09_examples"
    COMMAND
        Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "copylower"
        "${PROJECT_SOURCE_DIR}/CONTRIBUTING.md" "${DOCS_MARKDOWN_BINARY_DIR}/10_development"
    COMMAND
        Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "glob_readme"
        "${PROJECT_SOURCE_DIR}/tools" "${DOCS_MARKDOWN_BINARY_DIR}/14_additional"
    COMMAND
        Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "copylower"
        "${PROJECT_SOURCE_DIR}/AUTHORS.md" "${DOCS_MARKDOWN_BINARY_DIR}/15_appendix"
    COMMAND
        Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "copylower"
        "${DOCS_TESTS_CONF}" "${DOCS_MARKDOWN_BINARY_DIR}/15_appendix"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/convert"
    COMMENT "Generating source tree of the Markdown documentation"
    VERBATIM)
ADD_DEPENDENCIES(apsq_docs_markdown Python::Interpreter)

################################################
# Target to create Markdown file tree for hugo #
################################################

SET(DOCS_HUGO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/usermanual_hugo")

ADD_CUSTOM_TARGET(
    apsq_docs_hugo
    COMMAND
        ${CMAKE_COMMAND} -E remove_directory -f ${DOCS_HUGO_BINARY_DIR}
    COMMAND
        Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "convert_hugo"
        "${DOCS_MARKDOWN_BINARY_DIR}" "${DOCS_HUGO_BINARY_DIR}"
    DEPENDS apsq_docs_markdown
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/convert"
    COMMENT "Generating source tree of the hugo documentation"
    VERBATIM)
ADD_DEPENDENCIES(apsq_docs_hugo Python::Interpreter)

############################################################
# Target to create LaTex file tree for the PDF user manual #
############################################################

SET(DOCS_LATEX_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/usermanual_latex")

FIND_PACKAGE(Pandoc)
IF(PANDOC_FOUND)
    MESSAGE(STATUS "Docs: Adding target for user manual LaTeX generation")
    ADD_CUSTOM_TARGET(
        apsq_docs_latex
        COMMAND
            ${CMAKE_COMMAND} -E remove_directory -f ${DOCS_LATEX_BINARY_DIR}
        COMMAND
            Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "convert_latex"
            "${DOCS_MARKDOWN_BINARY_DIR}" "${DOCS_LATEX_BINARY_DIR}"
        DEPENDS apsq_docs_markdown
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/convert"
        COMMENT "Generating source tree of the LaTeX documentation"
        VERBATIM)
    ADD_DEPENDENCIES(apsq_docs_latex Python::Interpreter Pandoc::pandoc)
ENDIF()

###############################################
# LaTeX target to compile the PDF user manual #
###############################################

SET(DOCS_PDF_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/usermanual_pdf")

FIND_PACKAGE(Latexmk)
FIND_PACKAGE(Lualatex)
FIND_PACKAGE(Biber)
FIND_PACKAGE(Pygmentize)

IF(LATEXMK_FOUND AND LUALATEX_FOUND AND BIBER_FOUND AND PYGMENTIZE_FOUND)
    MESSAGE(STATUS "Docs: Adding target for user manual PDF generation")
    SET(LATEXMK_COMPILER_FLAGS "-pdflua" "-cd" "-shell-escape")
    SET(DOCS_REFERENCES_BIB "${CMAKE_CURRENT_SOURCE_DIR}/latex/references.bib")
    SET(DOCS_ALLPIX_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/logo.png")
    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/latex/frontmatter.tex" "${CMAKE_CURRENT_BINARY_DIR}/frontmatter.tex" @ONLY)

    ADD_CUSTOM_TARGET(
        apsq_docs_pdf
        COMMAND
            ${CMAKE_COMMAND} -E remove_directory -f ${DOCS_PDF_BINARY_DIR}
        COMMAND
            Python::Interpreter "-B" ${DOCS_PYTHON_HELPER} "create_latex_input"
            "${DOCS_LATEX_BINARY_DIR}" "${DOCS_PDF_BINARY_DIR}/input.tex"
        COMMAND
            ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/latex/allpix-manual.tex" "${DOCS_PDF_BINARY_DIR}"
        COMMAND
            ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/frontmatter.tex" "${DOCS_PDF_BINARY_DIR}"
        COMMAND
            Latexmk::latexmk ${LATEXMK_COMPILER_FLAGS} "${DOCS_PDF_BINARY_DIR}/allpix-manual.tex"
        COMMAND
            ${CMAKE_COMMAND} -E rename "${DOCS_PDF_BINARY_DIR}/allpix-manual.pdf"
            "${DOCS_PDF_BINARY_DIR}/allpix-manual-${CPACK_PACKAGE_VERSION}.pdf"
        DEPENDS apsq_docs_latex
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/convert"
        COMMENT "Generating the PDF documentation"
        VERBATIM)
    ADD_DEPENDENCIES(
        apsq_docs_pdf Python::Interpreter Latexmk::latexmk Lualatex::lualatex Biber::biber Pygmentize::pygmentize)
ENDIF()
