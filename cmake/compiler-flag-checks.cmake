# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

IF(NOT DEFINED ALLPIX_CXX_FLAGS)
    # Check for supported flags and remove unsupported warnings
    FOREACH(flag ${DEFAULT_ALLPIX_CXX_FLAGS})
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

    # Reduce Wstrict-overflow level for some GCC versions due to false positives:
    IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
            LIST(APPEND ALLPIX_CXX_FLAGS "-Wstrict-overflow=2")
        ENDIF()
    ENDIF()
ENDIF()

IF(NOT DEFINED ALLPIX_SHARED_LINKER_FLAGS)
    # Set no undefined symbols flag for the linker if supported
    IF((CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") OR (CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
        # NOTE: Apple Clang LD (ld-1267) warns undefined-error is deprecated; this
        # should probably be removed
        SET(ALLPIX_SHARED_LINKER_FLAGS "-Wl,-undefined,error")
    ELSEIF((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        SET(ALLPIX_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
    ENDIF()
ENDIF()
