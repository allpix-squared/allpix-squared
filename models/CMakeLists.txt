# Set the suffix for model files
SET(MODEL_SUFFIX ".conf" CACHE INTERNAL "Model files suffix" FORCE)

# Prefer local models if not installed to other location
GET_FILENAME_COMPONENT(ABSOLUTE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
IF(ABSOLUTE_INSTALL_PREFIX STREQUAL PROJECT_SOURCE_DIR)
    # Use local model directory (unless other is given)
    SET(MODEL_DIRECTORY "models" CACHE PATH "Models directory" FORCE)

    IF(NOT IS_ABSOLUTE ${MODEL_DIRECTORY})
        SET(MODEL_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${MODEL_DIRECTORY}" CACHE PATH "Models directory" FORCE)
    ENDIF()
ELSE()
    # Set the install path to share
    # FIXME Allow for more fine tuned install configuration
    SET(MODEL_DIRECTORY "share/${CMAKE_PROJECT_NAME}/models" CACHE PATH "Models directory" FORCE)
ENDIF()

# Install the files to the models directory
# NOTE: With default install path this does not change anything
INSTALL(
    FILES
    test.conf
    timepix.conf
    medipix3.conf
    mimosa23.conf
    mimosa26.conf
    fei3.conf
    ibl_planar.conf
    cmsp1.conf
    clicpix.conf
    clicpix2.conf
    DESTINATION ${MODEL_DIRECTORY}
    COMPONENT models)
