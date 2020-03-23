#!/bin/bash

# Determine which OS you are using
if [ "$(uname)" == "Linux" ]; then
    if [ "$( cat /etc/*-release | grep Scientific )" ]; then
        OS=slc6
    elif [ "$( cat /etc/*-release | grep CentOS )" ]; then
        OS=centos7
    else
        echo "Cannot detect OS, falling back to CentOS7"
        OS=centos7
    fi
elif [ "$(uname)" == "Darwin" ]; then
    if [ $(sw_vers -productVersion | awk -F '.' '{print $1 "." $2}') == "10.15" ]; then
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
if [ ! -d "/cvmfs" ]; then
    echo "No CVMFS detected, please install it."
    exit 1
fi

if [ ! -d "/cvmfs/sft.cern.ch" ]; then
    echo "No SFT CVMFS repository detected, please make sure it is available."
    exit 1
fi
if [ ! -d "/cvmfs/geant4.cern.ch" ]; then
    echo "No Geant4 CVMFS repository detected, please make sure it is available."
    exit 1
fi


# Determine which LCG version to use
if [ "$(uname)" == "Darwin" ]; then
    DEFAULT_LCG="LCG_97python3"
else
    DEFAULT_LCG="LCG_96b"
fi

if [ -z ${LCG_VERSION} ]; then
    echo "No explicit LCG version set, using ${DEFAULT_LCG}."
    LCG_VERSION=${DEFAULT_LCG}
fi

# Determine which compiler to use
if [ -z ${COMPILER_TYPE} ]; then
    if [ "$(uname)" == "Darwin" ]; then
        COMPILER_TYPE="llvm"
        echo "No compiler type set, falling back to AppleClang."
    else
        echo "No compiler type set, falling back to GCC."
        COMPILER_TYPE="gcc"
    fi
fi
if [ ${COMPILER_TYPE} == "gcc" ]; then
    COMPILER_VERSION="gcc8"
    echo "Compiler type set to GCC, version ${COMPILER_VERSION}."
fi
if [ ${COMPILER_TYPE} == "llvm" ]; then
    if [ "$(uname)" == "Darwin" ]; then
        COMPILER_VERSION="clang110"
    else
        COMPILER_VERSION="clang8"
    fi
    echo "Compiler type set to LLVM, version ${COMPILER_VERSION}."
fi

# Choose build type
if [ -z ${BUILD_TYPE} ]; then
    BUILD_TYPE=opt
fi

# General variables
SFTREPO=/cvmfs/sft.cern.ch
export BUILD_FLAVOUR=x86_64-${OS}-${COMPILER_VERSION}-${BUILD_TYPE}

#--------------------------------------------------------------------------------
#     Source full LCG view
#--------------------------------------------------------------------------------

export LCG_VIEW=${SFTREPO}/lcg/views/${LCG_VERSION}/${BUILD_FLAVOUR}/setup.sh
source ${LCG_VIEW} || echo "yes"

# Fix LCIO path for LCG_96, cmake configs are not properly placed:
export LCIO_DIR=$(dirname $(dirname $(readlink $(which lcio_event_counter))))

# Fix issue with wrongly picked-up compiler:
if [ "$(uname)" == "Darwin" ]; then
    unset CXX
    unset FC
fi
