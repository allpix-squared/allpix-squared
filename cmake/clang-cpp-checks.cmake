# Additional targets to perform clang-format/clang-tidy
# Get all project files
IF(NOT CHECK_CXX_SOURCE_FILES)
    MESSAGE(FATAL_ERROR "Variable CHECK_CXX_SOURCE_FILES not defined - set it to the list of files to lint and format")
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
    )

    ADD_CUSTOM_TARGET(
        check-format
        COMMAND ! ${CLANG_FORMAT}
        -style=file
        -output-replacements-xml
        ${CHECK_CXX_SOURCE_FILES}
        | grep -c "replacement " > /dev/null
    )
ENDIF()

# Adding clang-tidy target if executable is found
SET(CXX_STD 11)
IF(${CMAKE_CXX_STANDARD})
    SET(CXX_STD ${CMAKE_CXX_STANDARD})
ENDIF()

FIND_PROGRAM(CLANG_TIDY "clang-tidy")
IF(CLANG_TIDY)
    ADD_CUSTOM_TARGET(
        lint
        COMMAND ${CLANG_TIDY}
        -config=""
        -fix
        ${CHECK_CXX_SOURCE_FILES}
        --
        -std=c++${CXX_STD}
        ${INCLUDE_DIRECTORIES}
    )

    ADD_CUSTOM_TARGET(
        check-lint
        COMMAND ${CLANG_TIDY}
        -config=""
        ${CHECK_CXX_SOURCE_FILES}
        --
        -std=c++${CXX_STD}
        ${INCLUDE_DIRECTORIES}
    )
ENDIF()
