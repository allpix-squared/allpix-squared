#!/bin/bash

if [ "$(uname)" == "Darwin" ]; then
    if [ $(sw_vers -productVersion | awk -F '.' '{print $1 "." $2}') == "10.12" ]; then
        OS=mac1012
        COMPILER_TYPE=llvm
        COMPILER_VERSION=clang80
    else
        echo "Bootstrap only works on macOS Sierra (10.12)"
        exit 1
    fi
else 
    echo "This script is only meant for Mac"
    exit 1
fi

# Determine is you have CVMFS installed
if [ ! -d "/cvmfs" ]; then
    echo "No CVMFS detected, please install it."
    exit 1
fi

if [ ! -d "/cvmfs/clicdp.cern.ch" ]; then
    echo "No clicdp CVMFS repository detected, please add it."
    exit 1
fi

# Choose build type
if [ -z ${BUILD_TYPE} ]; then
    BUILD_TYPE=opt
fi


# General variables
CLICREPO=/cvmfs/clicdp.cern.ch
BUILD_FLAVOUR=x86_64-${OS}-${COMPILER_VERSION}-${BUILD_TYPE}

#--------------------------------------------------------------------------------
#     Python
#--------------------------------------------------------------------------------
export PYTHONDIR=${CLICREPO}/software/Python/2.7.12/${BUILD_FLAVOUR}
export PATH=${PYTHONDIR}/bin:$PATH
export DYLD_LIBRARY_PATH=${PYTHONDIR}/lib:${LD_LIBRARY_PATH}

#--------------------------------------------------------------------------------
#     CMake
#--------------------------------------------------------------------------------

export CMAKE_HOME=${CLICREPO}/software/CMake/3.6.2/${BUILD_FLAVOUR}
export PATH=${CMAKE_HOME}/bin:$PATH

#--------------------------------------------------------------------------------
#     Python
#--------------------------------------------------------------------------------

export PYTHONDIR=${CLICREPO}/software/Python/2.7.12/${BUILD_FLAVOUR}
export PATH=${PYTHONDIR}/bin:$PATH
export DYLD_LIBRARY_PATH="${PYTHONDIR}/lib:$DYLD_LIBRARY_PATH"

#--------------------------------------------------------------------------------
#     ROOT
#--------------------------------------------------------------------------------

export ROOTSYS=${CLICREPO}/software/ROOT/6.08.00/${BUILD_FLAVOUR}
export PYTHONPATH="$ROOTSYS/lib:$PYTHONPATH"
export PATH="$ROOTSYS/bin:$PATH"
export DYLD_LIBRARY_PATH="$ROOTSYS/lib:$DYLD_LIBRARY_PATH"

#--------------------------------------------------------------------------------
#     XercesC
#--------------------------------------------------------------------------------

export XercesC_HOME=${CLICREPO}/software/Xerces-C/3.1.4/${BUILD_FLAVOUR}
export PATH="$XercesC_HOME/bin:$PATH"
export DYLD_LIBRARY_PATH="$XercesC_HOME/lib:$DYLD_LIBRARY_PATH"


#--------------------------------------------------------------------------------
#     Geant4
#--------------------------------------------------------------------------------

export G4INSTALL=${CLICREPO}/software/Geant4/10.02.p02/${BUILD_FLAVOUR}
export G4LIB=$G4INSTALL/lib/Geant4-10.2.2/
export G4ENV_INIT="${G4INSTALL}/bin/geant4.sh"
export G4SYSTEM="Linux-g++"


#--------------------------------------------------------------------------------
#     Boost
#--------------------------------------------------------------------------------

export BOOST_ROOT=${CLICREPO}/software/Boost/1.62.0/${BUILD_FLAVOUR}
export DYLD_LIBRARY_PATH="${BOOST_ROOT}/lib:$DYLD_LIBRARY_PATH"

#--------------------------------------------------------------------------------
#     Ninja
#--------------------------------------------------------------------------------

export Ninja_HOME=${CLICREPO}/software/Ninja/1.7.1/${BUILD_FLAVOUR}
export PATH="$Ninja_HOME:$PATH"

#--------------------------------------------------------------------------------
#     Eigen
#--------------------------------------------------------------------------------

export Eigen_HOME=${CLICREPO}/software/Eigen/3.3.0/${BUILD_FLAVOUR}
export Eigen3_DIR=${Eigen_HOME}/lib64/cmake/eigen3/

#--------------------------------------------------------------------------------
#     LCIO
#--------------------------------------------------------------------------------

export LCIO=${CLICREPO}/software/LCIO/2.8.0/${BUILD_FLAVOUR}/
export CMAKE_PREFIX_PATH="$LCIO:$CMAKE_PREFIX_PATH"

