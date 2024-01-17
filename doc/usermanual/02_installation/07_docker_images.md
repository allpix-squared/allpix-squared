---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Docker Images"
weight: 7
---

Docker images are provided for the framework to allow anyone to run simulations without the need of installing Allpix Squared
on their system. The only required program is the Docker executable, all other dependencies are provided within the Docker
images. In order to exchange configuration files and output data between the host system and the Docker container, a folder
from the host system should be mounted to the container's data path `/data`, which also acts as the Docker `WORKDIR`
location.

The following command creates a container from the latest Docker image in the project registry and start an interactive shell
session with the `allpix` executable already in the `$PATH`. Here, the current host system path is mounted to the `/data`
directory of the container.

```shell
docker run --interactive --tty                                   \
           --volume "$(pwd)":/data                               \
           --name=allpix-squared                                 \
           gitlab-registry.cern.ch/allpix-squared/allpix-squared \
           bash
```

Alternatively it is also possible to directly start the simulation instead of an interactive shell, e.g. using the following
command:

```shell
docker run --tty --rm                                            \
           --volume "$(pwd)":/data                               \
           --name=allpix-squared                                 \
           gitlab-registry.cern.ch/allpix-squared/allpix-squared \
           "allpix -c my_simulation.conf"
```

where a simulation described in the configuration `my_simulation.conf` is directly executed and the container terminated and
deleted after completing the simulation. This closely resembles the behavior of running Allpix Squared natively on the host
system. Of course, any additional command line arguments known to the `allpix` executable described in
[Section 3.5](../03_getting_started/05_allpix_executable.md) can be appended.

For tagged versions, the tag name should be appended to the image name, e.g.
`gitlab-registry.cern.ch/allpix-squared/allpix-squared:v2.2.2`, and a full list of available Docker containers is provided
via the project's container registry \[[@ap2-container-registry]\]. A short description of how Docker images for this project
are built can be found in [Section 11.5](../11_devtools/05_building_docker_images.md).


[@ap2-container-registry]: https://gitlab.cern.ch/allpix-squared/allpix-squared/container_registry
