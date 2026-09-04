# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Check for supported flags and remove unsupported warnings
FOREACH(flag ${COMPILER_FLAGS})
    STRING(REPLACE "-" "_" FLAG_WORD ${flag})
    STRING(REPLACE "+" "P" FLAG_WORD ${FLAG_WORD})
    STRING(REPLACE "=" "E" FLAG_WORD ${FLAG_WORD})

    CHECK_CXX_COMPILER_FLAG("${flag}" CXX_FLAG_WORKS_${FLAG_WORD})
    IF(${CXX_FLAG_WORKS_${FLAG_WORD}})
        LIST(APPEND ALLPIX_CXX_FLAGS "${flag}")
    ELSE()
        MESSAGE(STATUS "NOT adding ${flag} to CXX_FLAGS - unsupported flag")
    ENDIF()
ENDFOREACH()

# Set no undefined symbols flag for the linker if supported
IF((CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") OR (CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
ELSEIF((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
ENDIF()

# Reduce Wstrict-overflow level for some GCC versions due to false positives:
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
        LIST(APPEND ALLPIX_CXX_FLAGS "-Wstrict-overflow=2")
    ENDIF()
ENDIF()
