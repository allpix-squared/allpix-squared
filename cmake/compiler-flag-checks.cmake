# Check for supported flags and remove unsupported warnings
INCLUDE(CheckCXXCompilerFlag)
FOREACH( FLAG ${COMPILER_FLAGS} )
    STRING(REPLACE "-" "_" FLAG_WORD ${FLAG} )
    STRING(REPLACE "+" "P" FLAG_WORD ${FLAG_WORD} )
    STRING(REPLACE "=" "E" FLAG_WORD ${FLAG_WORD} )

    CHECK_CXX_COMPILER_FLAG( "${FLAG}" CXX_FLAG_WORKS_${FLAG_WORD} )
    IF( ${CXX_FLAG_WORKS_${FLAG_WORD}} )
        SET ( CMAKE_CXX_FLAGS "${FLAG} ${CMAKE_CXX_FLAGS}")
    ELSE()
        MESSAGE ( STATUS "NOT adding ${FLAG} to CXX_FLAGS - unsupported flag" )
    ENDIF()
ENDFOREACH()

# Find threading provider and enable it (NOTE: not used yet)
IF( THREADS_HAVE_PTHREAD_ARG )
    SET( CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} -pthread")
    SET( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pthread")
ELSEIF( CMAKE_THREAD_LIBS_INIT )
    SET( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_THREAD_LIBS_INIT}")
ENDIF()

# Set no undefined symbols flag for the linker if supported
IF((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    SET ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
ENDIF()
IF(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    SET ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
ENDIF()

# Reduce Wstrict-overflow level for some GCC versions due to false positives:
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
     SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-overflow=2")
  ENDIF()
ENDIF()
