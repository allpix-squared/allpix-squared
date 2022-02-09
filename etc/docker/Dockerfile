# SPDX-FileCopyrightText: 2018-2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

FROM gitlab-registry.cern.ch/allpix-squared/allpix-squared/allpix-squared-deps:latest
MAINTAINER Simon Spannagel <simon.spannagel@cern.ch>
ARG ALLPIX_BUILD_ENV="Docker"

# Copy currently checked out git revision:
COPY . /opt/allpix-squared

ENV PATH="/opt/allpix-squared/bin:${PATH}"
WORKDIR "/data"

# Compile Allpix Squared
RUN mkdir -p /opt/allpix-squared/build && \
    cd /opt/allpix-squared/build && \
    cmake -DCMAKE_INSTALL_PREFIX=../ \
          -DBUILD_ALL_MODULES=ON \
          .. && \
    make -j`grep -c processor /proc/cpuinfo` && \
    make install && \
    rm -rf /opt/allpix-squared/build

# Default command for starting the container, executed after the ENTRYPOINT
CMD ["bash"]
