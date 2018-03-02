AllpixSquared_version="@ALLPIX_VERSION@"

# Dependencies
@SETUP_FILE_DEPS@

THIS_BASH=$BASH
if [[ $THIS_BASH == "" ]]; then
    THIS_HERE=$(dirname $0)
else
    THIS_HERE=$(dirname ${BASH_SOURCE[0]})
fi

CLIC_AllpixSquared_home=$THIS_HERE

# Export path for executable, library path and data path for detector models
export PATH=${CLIC_AllpixSquared_home}/bin:${PATH}
export LD_LIBRARY_PATH=${CLIC_AllpixSquared_home}/lib:${LD_LIBRARY_PATH}
export XDG_DATA_DIRS=${CLIC_AllpixSquared_home}/share:${XDG_DATA_DIRS}
