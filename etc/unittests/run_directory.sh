ABSOLUTE_PATH="$( cd "$( dirname "${BASH_SOURCE}" )" && pwd )"

# Load dependencies if run by the CI
# FIXME: This is needed because of broken RPATH on Mac
if [ -n "${CI}" ]; then
    if [ "$(uname)" == "Darwin" ]; then
        source $ABSOLUTE_PATH/../../.gitlab-ci.d/init_mac.sh
    else
        source $ABSOLUTE_PATH/../../.gitlab-ci.d/init_x86_64.sh
    fi
    source $ABSOLUTE_PATH/../../.gitlab-ci.d/load_deps.sh
fi

# Run the second argument in the directory created from the first argument
rm -rf $1
mkdir -p $1
cd $1
exec $2
