#!/bin/bash

# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Determine which OS you are using
if [ "$(uname)" = "Linux" ]; then
    if [ "$( cat /etc/*-release | grep "Red Hat Enterprise Linux 9" )" ]; then
        echo "Detected Red Hat Enterprise Linux 9"
        OS=el9
    elif [ "$( cat /etc/*-release | grep "Ubuntu 24.04" )" ]; then
        echo "Detected Ubuntu 24.04"
        OS=ubuntu2404
    else
        echo "Cannot detect OS, falling back to RHEL9"
        OS=el9
    fi
elif [ "$(uname)" = "Darwin" ]; then
    MACOS_MAJOR=$(sw_vers -productVersion | awk -F '.' '{print $1}')
    MACOS_MINOR=$(sw_vers -productVersion | awk -F '.' '{print $2}')
    if [ $MACOS_MAJOR = "15" ]; then
        OS=mac15
    elif [ $MACOS_MAJOR = "14" ]; then
        OS=mac14
    elif [ $MACOS_MAJOR = "13" ]; then
        OS=mac13
    else
        echo "Unsupported version of macOS ${MACOS_MAJOR}.${MACOS_MINOR}"
        exit 1
    fi
else
    echo "Unknown OS"
    exit 1
fi

# Determine is you have CVMFS installed
CVMFS_MOUNT=""
if [ "${OS:0:3}" = mac ]; then
    CVMFS_MOUNT="/Users/Shared"
fi

if [ ! -d "${CVMFS_MOUNT}/cvmfs" ]; then
    echo "No CVMFS detected, please install it. Looking at ${CVMFS_MOUNT}/cvmfs"
    exit 1
fi

if [ ! -d "${CVMFS_MOUNT}/cvmfs/sft.cern.ch" ]; then
    echo "No SFT CVMFS repository detected, please make sure it is available."
    exit 1
fi
if [ ! -d "${CVMFS_MOUNT}/cvmfs/geant4.cern.ch" ]; then
    echo "No Geant4 CVMFS repository detected, please make sure it is available."
    exit 1
fi


# Determine which LCG version to use
DEFAULT_LCG="LCG_108"

if [ -z ${ALLPIX_LCG_VERSION} ]; then
    echo "No explicit LCG version set, using ${DEFAULT_LCG}."
    ALLPIX_LCG_VERSION=${DEFAULT_LCG}
fi

# Determine which compiler to use
if [ -z ${COMPILER_TYPE} ]; then
    if [ "$(uname)" = "Darwin" ]; then
        COMPILER_TYPE="llvm"
        echo "No compiler type set, falling back to AppleClang."
    else
        echo "No compiler type set, falling back to GCC."
        COMPILER_TYPE="gcc"
    fi
fi
if [ ${COMPILER_TYPE} = "gcc" ]; then
    if [ "$OS" = ubuntu2404 ]; then
        COMPILER_VERSION="gcc13"
    else
        COMPILER_VERSION="gcc15"
    fi
    echo "Compiler type set to GCC, version ${COMPILER_VERSION}."
fi
if [ ${COMPILER_TYPE} = "llvm" ]; then
    if [ "$(uname)" = "Darwin" ]; then
        if [ "$OS" = mac13 ]; then
            COMPILER_VERSION="clang150"
        elif [ "$OS" = mac14 ]; then
            COMPILER_VERSION="clang160"
        else
            COMPILER_VERSION="clang170"
        fi
    else
        COMPILER_VERSION="clang19"
    fi
    echo "Compiler type set to LLVM, version ${COMPILER_VERSION}."
fi

# Choose build type
if [ -z ${BUILD_TYPE} ]; then
    BUILD_TYPE=opt
fi

# General variables
SFTREPO=${CVMFS_MOUNT}/cvmfs/sft.cern.ch
export BUILD_FLAVOUR=x86_64-${OS}-${COMPILER_VERSION}-${BUILD_TYPE}

#--------------------------------------------------------------------------------
#     Source dependencies
#--------------------------------------------------------------------------------

export LCG_VIEW=${SFTREPO}/lcg/views/${ALLPIX_LCG_VERSION}/${BUILD_FLAVOUR}/setup.sh
source ${LCG_VIEW} || echo "yes"

if [ -n "${CI}" ] && [ "$(uname)" = "Darwin" ]; then
    source $ROOTSYS/bin/thisroot.sh
    cd $G4INSTALL/bin/
    source geant4.sh
    cd -
fi
