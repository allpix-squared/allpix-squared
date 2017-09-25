# Only install macros if installation directory is outside the source directory
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(NOT ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR)
    # Use local model directory (unless other is given)
    SET(MACRO_DIRECTORY "share/${CMAKE_PROJECT_NAME}/macros" CACHE PATH "ROOT macros directory")
    IF(NOT IS_ABSOLUTE ${MACRO_DIRECTORY})
        SET(MACRO_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${MACRO_DIRECTORY}" CACHE PATH "ROOT macros directory" FORCE)
    ENDIF()

    # Install the files to the models directory
    # NOTE: With default install path this does not change anything
    INSTALL(
        FILES
        constructComparisonTree.C
        DESTINATION ${MACRO_DIRECTORY})
ENDIF()
