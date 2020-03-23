#!/bin/bash

# Determine which OS you are using
if [ "$(uname)" == "Linux" ]; then
    if [ "$( cat /etc/*-release | grep Scientific )" ]; then
        OS=slc6
    elif [ "$( cat /etc/*-release | grep CentOS )" ]; then
        OS=centos7
    else
        echo "Cannot detect OS, falling back to SLC6"
        OS=slc6
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
if [ -z ${LCG_VERSION} ]; then
    echo "No explicit LCG version set, using LCG_96b."
    LCG_VERSION="LCG_96b"
fi

# Determine which compiler to use
if [ -z ${COMPILER_TYPE} ]; then
    echo "No compiler type set, falling back to GCC."
    COMPILER_TYPE="gcc"
fi
if [ ${COMPILER_TYPE} == "gcc" ]; then
    echo "Compiler type set to GCC."
    COMPILER_VERSION="gcc8"
fi
if [ ${COMPILER_TYPE} == "llvm" ]; then
    echo "Compiler type set to LLVM."
    COMPILER_VERSION="clang8"
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
source ${LCG_VIEW}

# Fix LCIO path for LCG_96, cmake configs are not properly placed:
export LCIO_DIR=$(dirname $(dirname $(readlink $(which lcio_event_counter))))
