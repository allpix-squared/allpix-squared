# SPDX-FileCopyrightText: 2018-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

AllpixSquared_version="@ALLPIX_VERSION@"

# Dependencies - directory changes required for zsh compatibility
@SETUP_FILE_DEPS@

THIS_BASH=$BASH
if [[ $THIS_BASH == "" ]]; then
    THIS_HERE=$(dirname $0)
else
    THIS_HERE=$(dirname ${BASH_SOURCE[0]})
fi

CLIC_AllpixSquared_home=$THIS_HERE

# Export path for executable, library path, data path for detector models and headers for ROOT objects
export PATH=${CLIC_AllpixSquared_home}/bin:${PATH}
export LD_LIBRARY_PATH=${CLIC_AllpixSquared_home}/lib:${LD_LIBRARY_PATH}
export XDG_DATA_DIRS=${CLIC_AllpixSquared_home}/share:${XDG_DATA_DIRS}
export CMAKE_PREFIX_PATH=${CLIC_AllpixSquared_home}/share/cmake:$CMAKE_PREFIX_PATH
export ROOT_INCLUDE_PATH=${CLIC_AllpixSquared_home}/include/objects:$ROOT_INCLUDE_PATH
