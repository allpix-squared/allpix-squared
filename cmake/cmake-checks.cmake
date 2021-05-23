# Additional targets to perform cmake-lint/cmake-format

FILE(
    GLOB_RECURSE CHECK_CMAKE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${PROJECT_SOURCE_DIR}
    "cmake/*.cmake" "CMakeLists.txt")
LIST(REMOVE_ITEM CHECK_CMAKE_FILES "cmake/LATEX.cmake" "cmake/PANDOC.cmake" "cmake/FindPQXX.cmake"
     "cmake/CodeCoverage.cmake")

FIND_PROGRAM(CMAKELINT "cmake-lint")
IF(CMAKELINT)
    ADD_CUSTOM_TARGET(
        lint-cmake
        COMMAND ${CMAKELINT} ${CHECK_CMAKE_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Run linter check on CMake code")
ENDIF()

FIND_PROGRAM(CMAKEFORMAT "cmake-format")
IF(CMAKEFORMAT)
    ADD_CUSTOM_TARGET(
        format-cmake
        COMMAND ${CMAKEFORMAT} -i ${CHECK_CMAKE_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Run formatting tool on CMake code")
ENDIF()
