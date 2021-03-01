#!/usr/bin/env bash

mkdir build
cd build
cmake -GNinja -DCMAKE_CXX_FLAGS="-Werror" -DBUILD_LCIOWriter=ON -DCMAKE_BUILD_TYPE=RELEASE -DROOT_DIR=$ROOTSYS -DEigen3_DIR=$Eigen3_DIR -DLCIO_DIR=$LCIO_DIR ..
make install
ninja -k0
ninja install
