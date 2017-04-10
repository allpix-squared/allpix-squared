#!/bin/bash
cd /Package
mkdir build
cd build
source ../.gitlab-ci.d/init_x86_64.sh
cmake -DCMAKE_CXX_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=DEBUG .. && \
export PATH=/Package/cov-analysis-linux64/bin:$PATH && \
cov-build --dir cov-int make -j2 && \
tar czvf myproject.tgz cov-int
