# FIXME: should not be necessary
FIND_PACKAGE( PkgConfig )
pkg_check_modules( EIGEN3 REQUIRED eigen3 )
INCLUDE_DIRECTORIES(SYSTEM ${EIGEN3_INCLUDE_DIRS} )

#INCLUDE_DIRECTORIES(SYSTEM ../core)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")

ADD_EXECUTABLE(allpix allpix.cpp)
TARGET_LINK_LIBRARIES(allpix AllpixCore)

# FIXME: should not be needed
TARGET_LINK_LIBRARIES(allpix geometry_test)
TARGET_LINK_LIBRARIES(allpix deposition_simple)
TARGET_LINK_LIBRARIES(allpix visualization_test)

INSTALL(TARGETS allpix
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
