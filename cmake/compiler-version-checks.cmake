# SPDX-FileCopyrightText: 2020-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# General overview for compiler support of CXX language and library features:
# https://en.cppreference.com/w/cpp/compiler_support
#
# TODO: at some point this should probably be replaced by a proper use of CMAKE_CXX_COMPILE_FEATURES
# https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html

# Minimum GCC versions for C++17 feature support.
# based on
# https://gcc.gnu.org/projects/cxx-status.html
# https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2017
# Note: Version >=9 required by MagicEnum
SET(GCC_VERSION_MIN "9.0")

# Minimum Clang versions for C++17 feature support.
# based on
# https://clang.llvm.org/cxx_status.html
SET(CLANG_VERSION_MIN "7.0")

# Minimum Apple Clang versions for C++17 feature support.
# based on
# https://trac.macports.org/wiki/XcodeVersionInfo
SET(APPLECLANG_VERSION_MIN "10.0")

IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS GCC_VERSION_MIN)
        MESSAGE(FATAL_ERROR "Requiring at least GCC version ${GCC_VERSION_MIN}. Available version"
                            " ${CMAKE_CXX_COMPILER_VERSION} does not fully support required C++ features")
    ENDIF()
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CLANG_VERSION_MIN)
        MESSAGE(FATAL_ERROR "Requiring at least Clang version ${CLANG_VERSION_MIN}. Available version"
                            " ${CMAKE_CXX_COMPILER_VERSION} does not fully support required C++ features")
    ENDIF()
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS APPLECLANG_VERSION_MIN)
        MESSAGE(FATAL_ERROR "Requiring at least AppleClang version ${APPLECLANG_VERSION_MIN}. Available version"
                            " ${CMAKE_CXX_COMPILER_VERSION} does not fully support required C++ features")
    ENDIF()
ENDIF()
