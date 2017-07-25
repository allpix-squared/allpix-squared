if [ ! -d "/cvmfs/sft.cern.ch" ]; then
    echo "CVMFS not available"
    return
fi

# Get our directory and load the CI init
ABSOLUTE_PATH=`dirname $(readlink -f ${BASH_SOURCE[0]})`

# Set the compiler type to LLVM to also load clang-format and clang-tidy
export COMPILER_TYPE="llvm"

# Load default configuration
source $ABSOLUTE_PATH/../../.gitlab-ci.d/init_x86_64.sh

# Load different Geant for the moment, because CLICdp version does not have QT
# FIXME: This should not be a fixed directory
export G4INSTALL=/cvmfs/sft.cern.ch/lcg/releases/Geant4/10.03.p01-8919f/x86_64-slc6-gcc62-opt/
source $G4INSTALL/bin/geant4.sh
export CMAKE_PREFIX_PATH="$G4INSTALL:$CMAKE_PREFIX_PATH"
# Point to corresponding CLHEP
export CMAKE_PREFIX_PATH="/cvmfs/sft.cern.ch/lcg/releases/clhep/2.3.4.4-adaae/x86_64-slc6-gcc62-opt:$CMAKE_PREFIX_PATH"

