# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

if [ ! -d "/cvmfs/sft.cern.ch" ]; then
    echo "CVMFS not available"
    return
fi

# Get our directory and load the CI init
ABSOLUTE_PATH=`dirname $(readlink -f ${BASH_SOURCE[0]})`

# Set the compiler type to LLVM to also load clang-format and clang-tidy
if [ "$( cat /etc/*-release | grep "Red Hat Enterprise Linux 9" )" ] || [ "$( cat /etc/*-release | grep "AlmaLinux 9" )" ]; then
    export COMPILER_TYPE="llvm"
else
    export COMPILER_TYPE="gcc"
fi


# Load default configuration
source $ABSOLUTE_PATH/../../.ci/init_x86_64.sh
