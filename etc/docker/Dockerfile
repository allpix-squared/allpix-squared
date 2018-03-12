FROM gitlab-registry.cern.ch/allpix-squared/allpix-squared/allpix-squared-deps:latest
MAINTAINER Simon Spannagel <simon.spannagel@cern.ch>

# Copy currently checked out git revision:
COPY . /opt/allpix-squared

ENV PATH="/opt/allpix-squared/bin:${PATH}"
WORKDIR "/data"

# Compile Allpix Squared
RUN source scl_source enable devtoolset-7 && \
    mkdir -p /opt/allpix-squared/build && \
    cd /opt/allpix-squared/build && \
    cmake3 -DCMAKE_INSTALL_PREFIX=../ \
           -DCMAKE_MODULE_PATH="/usr/share/cmake3/Modules/;/usr/share/cmake/Modules/" \
          .. && \
    make -j`grep -c processor /proc/cpuinfo` && \
    make install && \
    rm -rf /opt/allpix-squared/build

# Default command for starting the container, executed after the ENTRYPOINT
CMD ["bash"]
