if [ ! -d "/cvmfs/sft.cern.ch" ]; then
    echo "CVMFS not available"
    return
fi

# Get our directory and load the CI init
ABSOLUTE_PATH=`dirname $(readlink -f ${BASH_SOURCE[0]})`

# Load default configuration
source $ABSOLUTE_PATH/../../.gitlab-ci.d/init_x86_64.sh

# Load different Geant for the moment, because CLICdp version does not have QT
# FIXME: This should not be a fixed directory
source /cvmfs/sft.cern.ch/lcg/releases/Geant4/10.02.p02-63aaa/x86_64-slc6-gcc62-opt/bin/geant4.sh

# Set special CMake include file to set other options
export ALLPIX_CMAKE_INCLUDE_FILE=$ABSOLUTE_PATH/lxplus.cmake
