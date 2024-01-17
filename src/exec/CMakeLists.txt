# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# include dependencies
INCLUDE_DIRECTORIES(SYSTEM ${ALLPIX_DEPS_INCLUDE_DIRS})

# create executable and link the libs
ADD_EXECUTABLE(allpix allpix.cpp)
TARGET_LINK_LIBRARIES(allpix ${ALLPIX_LIBRARIES})

# prelink all module libraries
# NOTE: fixes both the RPATH problem as well as the TLS problems
# FIXME: should be removed when we have a better solution
TARGET_LINK_LIBRARIES(allpix ${_ALLPIX_MODULE_LIBRARIES})

# set install location
INSTALL(
    TARGETS allpix
    EXPORT allpix_install
    COMPONENT application
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
