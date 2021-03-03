if [ ! -d "/cvmfs/sft.cern.ch" ]; then
    echo "CVMFS not available"
    return
fi

# Get our directory and load the CI init
ABSOLUTE_PATH=`dirname $(readlink -f ${BASH_SOURCE[0]})`

# Set the compiler type to LLVM to also load clang-format and clang-tidy
export COMPILER_TYPE="llvm"

# Load default configuration
source $ABSOLUTE_PATH/../../etc/scripts/.init_x86_64.sh

