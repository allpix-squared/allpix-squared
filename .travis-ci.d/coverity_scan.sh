#!/bin/bash
cd /Package
mkdir build
cd build
source ../etc/scripts/.init_x86_64.sh
cmake -DBUILD_ALL_MODULES=ON -DGeant4_DIR=$G4LIB -DROOT_DIR=$ROOTSYS -DEigen3_DIR=$Eigen3_DIR .. && \
export PATH=/Package/cov-analysis-linux64/bin:$PATH && \
cov-build --dir cov-int make -j2 && \
tar czvf myproject.tgz cov-int
