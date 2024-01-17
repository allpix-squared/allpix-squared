#!/bin/bash

# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

ABSOLUTE_PATH="$( cd "$( dirname "${BASH_SOURCE}" )" && pwd )"

# Load dependencies if run by the CI
# FIXME: This is needed because of broken RPATH on Mac
if [ -n "${CI}" ] && [ "$(uname)" == "Darwin" ]; then
    source $ABSOLUTE_PATH/../../.ci/init_x86_64.sh
fi

echo "Moving to WORKINGDIR"
echo "$1"
rm -rf $1
mkdir -p $1
cd $1
pwd

# Run all additional arguments as script commands before the test
if [ "$#" -gt 2 ]; then
    echo "Running BEFORE_SCRIPT"
    for cmd in "${@:3}"; do
        echo "${cmd}"
        ${cmd}
    done
fi

# Run the second argument in the directory created from the first argument
echo "Running TEST"
echo "$2"
exec $2
