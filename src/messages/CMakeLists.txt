SET(CMAKE_CXX_FLAGS "-g -Wall -Wextra -pedantic -Wshadow -Wformat-security -Wno-long-long -Wdeprecated -fdiagnostics-color=auto -Winconsistent-missing-override -Weffc++ ${CMAKE_CXX_FLAGS}")

FIND_PACKAGE(ROOT REQUIRED)
IF(NOT ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
    "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF(NOT ROOT_FOUND)

INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIRS})

ADD_LIBRARY(AllpixMessages SHARED
    DepositionMessage.cpp
)

TARGET_LINK_LIBRARIES(AllpixMessages ${ROOT_LIBRARIES})

INSTALL(TARGETS AllpixMessages
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
