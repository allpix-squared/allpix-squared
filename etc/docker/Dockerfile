# SPDX-FileCopyrightText: 2018-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

FROM gitlab-registry.cern.ch/allpix-squared/allpix-squared/allpix-squared-deps:v6
MAINTAINER Simon Spannagel <simon.spannagel@cern.ch>
ARG ALLPIX_BUILD_ENV="Docker"

# Copy currently checked out git revision:
COPY . /tmp/allpix-squared

ENV ROOT_INCLUDE_PATH=/usr/local/include/objects
WORKDIR "/data"

# Compile Allpix Squared, install to /usr/local
RUN mkdir -p /tmp/allpix-squared/build && \
    cd /tmp/allpix-squared/build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ \
          -DBUILD_DatabaseWriter=ON \
          -DBUILD_LCIOWriter=ON \
          -DBUILD_VisualizationGeant4=OFF \
          .. && \
    make -j`grep -c processor /proc/cpuinfo` && \
    make install && \
    rm -rf /tmp/allpix-squared

# Default command for starting the container, executed after the ENTRYPOINT
CMD ["bash"]
