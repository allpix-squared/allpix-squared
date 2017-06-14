# Set the suffix for model files
SET(MODEL_SUFFIX ".conf" CACHE INTERNAL "Model files suffix" FORCE)

# Prefer local models if not installed to other location
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR)
    # use local model directory (unless other is given)
    SET(MODEL_DIRECTORY "models" CACHE PATH "Models directory")
    IF(NOT IS_ABSOLUTE ${MODEL_DIRECTORY})
        SET(MODEL_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${MODEL_DIRECTORY}" CACHE PATH "Models directory" FORCE)
    ENDIF()
ELSE()
    # set the install path to share
    # FIXME: allow for more fine tuned install configuration
    SET(MODEL_DIRECTORY "share/${CMAKE_PROJECT_NAME}/" CACHE PATH "Models directory")
    IF(NOT IS_ABSOLUTE ${MODEL_DIRECTORY})
        SET(MODEL_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${MODEL_DIRECTORY}" CACHE PATH "Models directory" FORCE)
    ENDIF()
ENDIF()

# Install the files to the models directory
# NOTE: With default install path this does not change anything
INSTALL(
    FILES
    test.conf
    timepix.conf
    DESTINATION ${MODEL_DIRECTORY})
