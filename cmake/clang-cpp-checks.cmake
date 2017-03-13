# Additional targets to perform clang-format/clang-tidy

# Get all project files - FIXME: this should also use the list of generated targets
IF(NOT CHECK_CXX_SOURCE_FILES)
    MESSAGE(FATAL_ERROR "Variable CHECK_CXX_SOURCE_FILES not defined - set it to the list of files format")
    RETURN()
ENDIF()

# Adding clang-format check and formatter if found
FIND_PROGRAM(CLANG_FORMAT "clang-format")
IF(CLANG_FORMAT)
    ADD_CUSTOM_TARGET(
        format
        COMMAND ${CLANG_FORMAT}
        -i
        -style=file
        ${CHECK_CXX_SOURCE_FILES}
        COMMENT "Auto formatting of all source files"
    )

    ADD_CUSTOM_TARGET(
        check-format
        COMMAND ! ${CLANG_FORMAT}
        -style=file
        -output-replacements-xml
        ${CHECK_CXX_SOURCE_FILES}
        | grep -c "replacement " > /dev/null
        COMMENT "Checking format compliance"
    )
ENDIF()

# Adding clang-tidy target if executable is found
SET(CXX_STD 11)
IF(${CMAKE_CXX_STANDARD})
    SET(CXX_STD ${CMAKE_CXX_STANDARD})
ENDIF()

FIND_PROGRAM(CLANG_TIDY "clang-tidy")
# enable clang tidy only if using a clang compiler
IF(CLANG_TIDY AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    SET(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} "-header-filter='${CMAKE_SOURCE_DIR}'")
    MESSAGE(${CMAKE_CXX_CLANG_TIDY})

    # enable checking and formatting through run-clang-tidy if available
    # FIXME: make finding this program more portable
    GET_FILENAME_COMPONENT(CLANG_DIR ${CLANG_TIDY} DIRECTORY)
    FIND_PROGRAM(RUN_CLANG_TIDY "run-clang-tidy.py" HINTS /usr/share/clang/ ${CLANG_DIR}/../share/clang/)
    IF(RUN_CLANG_TIDY)
        # Set export commands on
        SET (CMAKE_EXPORT_COMPILE_COMMANDS ON)

        ADD_CUSTOM_TARGET(
            lint COMMAND
            ${RUN_CLANG_TIDY} -fix -format -header-filter=${CMAKE_SOURCE_DIR}
        )

        ADD_CUSTOM_TARGET(
            check-lint COMMAND
            ${RUN_CLANG_TIDY} -header-filter=${CMAKE_SOURCE_DIR}
        )
    ENDIF()
ENDIF()
