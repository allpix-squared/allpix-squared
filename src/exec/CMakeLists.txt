# FIXME: should not be necessary
FIND_PACKAGE(Eigen3 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${EIGEN3_INCLUDE_DIR})

#INCLUDE_DIRECTORIES(SYSTEM ../core)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "-g -Wall -Wextra -pedantic -Wshadow -Wformat-security -Wno-long-long -Wdeprecated -fdiagnostics-color=auto -Winconsistent-missing-override -Weffc++ -Weffc++ ${CMAKE_CXX_FLAGS}")

ADD_EXECUTABLE(allpix allpix.cpp)
TARGET_LINK_LIBRARIES(allpix AllpixCore)

# FIXME: should not be needed
TARGET_LINK_LIBRARIES(allpix geometry_test)
TARGET_LINK_LIBRARIES(allpix deposition_simple)
TARGET_LINK_LIBRARIES(allpix visualization_test)
TARGET_LINK_LIBRARIES(allpix deposit_reader_test)

INSTALL(TARGETS allpix
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
