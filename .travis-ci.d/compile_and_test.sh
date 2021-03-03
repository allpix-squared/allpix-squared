#!/bin/bash
cd /Package
mkdir build
cd build
source ../etc/scripts/.init_x86_64.sh
source ../.gitlab/ci/load_deps.sh
cmake -GNinja -DBUILD_ALL_MODULES=ON -DGeant4_DIR=$G4LIB -DROOT_DIR=$ROOTSYS -DEigen3_DIR=$Eigen3_DIR .. && \
ninja && \
ninja install && \
ninja check-format && \
ctest -E test_performance --output-on-failure -j4
