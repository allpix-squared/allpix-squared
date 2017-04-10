#!/bin/bash
cd /Package
mkdir build
cd build
source ../.gitlab-ci.d/init_x86_64.sh
cmake -DCMAKE_BUILD_TYPE=RELEASE -DGeant4_DIR=$G4INSTALL/lib64/Geant4-10.2.2 -DROOT_DIR=$ROOTSYS -DEigen3_DIR=$Eigen_HOME/lib64/cmake/eigen3 .. && \
export PATH=/Package/cov-analysis-linux64/bin:$PATH && \
cov-build --dir cov-int make -j2 && \
tar czvf myproject.tgz cov-int
