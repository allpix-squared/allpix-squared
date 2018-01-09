# Only install macros if installation directory is outside the source directory
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR)
    # Use local macro directory (unless other is given)
    SET(MACRO_DIRECTORY "tools/root_analysis_macros" CACHE PATH "ROOT macros directory" FORCE)
ELSE()
    # Set the install path to share
    # FIXME Allow for more fine tuned install configuration
    SET(MACRO_DIRECTORY "share/${CMAKE_PROJECT_NAME}/macros" CACHE PATH "ROOT macros directory" FORCE)
ENDIF()

# Install the files to the models directory
# NOTE: With default install path this does not change anything
INSTALL(
    FILES
    constructComparisonTree.C
    remakeProject.C
    DESTINATION ${MACRO_DIRECTORY}
    COMPONENT tools)
