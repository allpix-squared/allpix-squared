#!/bin/bash

# Determine which OS you are using
if [ "$(uname)" = "Linux" ]; then
    if [ "$( cat /etc/*-release | grep Scientific )" ]; then
        OS=slc6
    elif [ "$( cat /etc/*-release | grep CentOS )" ]; then
        OS=centos7
    else
        echo "Cannot detect OS, falling back to CentOS7"
        OS=centos7
    fi
elif [ "$(uname)" = "Darwin" ]; then
    if [ $(sw_vers -productVersion | awk -F '.' '{print $1 "." $2}') = "10.15" ]; then
        OS=mac1015
    else
        echo "Bootstrap only works on macOS Catalina (10.15)"
        exit 1
    fi
else
    echo "Unknown OS"
    exit 1
fi

# Determine is you have CVMFS installed
if [ "$OS" = mac1015 ]; then
    CVMFS_MOUNT="/Users/Shared"
else
    CVMFS_MOUNT=""
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
DEFAULT_LCG="LCG_98python3"

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
    if [ ${OS} = "slc6" ]; then
        COMPILER_VERSION="gcc8"
    else
        COMPILER_VERSION="gcc10"
    fi
    echo "Compiler type set to GCC, version ${COMPILER_VERSION}."
fi
if [ ${COMPILER_TYPE} = "llvm" ]; then
    if [ "$(uname)" = "Darwin" ]; then
        COMPILER_VERSION="clang110"
    else
        COMPILER_VERSION="clang10"
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

# Fix LCIO path for LCG_96, cmake configs are not properly placed:
export LCIO_DIR=$(dirname $(dirname $(readlink $(which lcio_event_counter))))
