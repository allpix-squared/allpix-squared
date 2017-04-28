# set the suffix for model files
SET(ALLPIX_MODEL_SUFFIX ".ini" CACHE INTERNAL "Model files suffix" FORCE)

# prefer local models if not installed to other location
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR)
    # use local model directory (unless other is given)
    SET(ALLPIX_MODEL_DIRECTORY "models" CACHE PATH "Models directory")
    IF(NOT IS_ABSOLUTE ${ALLPIX_MODEL_DIRECTORY})
        SET(ALLPIX_MODEL_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${ALLPIX_MODEL_DIRECTORY}" CACHE PATH "Models directory" FORCE)
    ENDIF()
ELSE()
    # set the install path to share
    # FIXME: allow for more fine tuned install configuration
    SET(ALLPIX_MODEL_DIRECTORY "share/${CMAKE_PROJECT_NAME}/" CACHE PATH "Models directory")
    IF(NOT IS_ABSOLUTE ${ALLPIX_MODEL_DIRECTORY})
        SET(ALLPIX_MODEL_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${ALLPIX_MODEL_DIRECTORY}" CACHE PATH "Models directory" FORCE)
    ENDIF()
ENDIF()

# install the files to the models directory
# NOTE: with default install path this does not change anything
INSTALL(
    FILES
    test.ini
    timepix.ini
    DESTINATION ${ALLPIX_MODEL_DIRECTORY})
