SET(CMAKE_CXX_FLAGS "-std=c++14 -O2 -Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")

# FIXME: not necessary
find_package( PkgConfig )
pkg_check_modules( EIGEN3 REQUIRED eigen3 )
include_directories(SYSTEM ${EIGEN3_INCLUDE_DIRS} )

ADD_EXECUTABLE(allpix allpix.cpp)
TARGET_LINK_LIBRARIES(allpix AllpixCore)

INSTALL(TARGETS allpix
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
