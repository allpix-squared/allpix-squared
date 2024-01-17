---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Building Docker Images"
weight: 5
---

New Allpix Squared Docker images are automatically created and deployed by the CI for every new tag and as a nightly build
from the `master` branch. New versions are published to project Docker container registry \[[@ap2-container-registry]\].
Tagged versions can be found via their respective tag name, while updates via the nightly build are always stored with the
`latest` tag attached.

The final Docker image is formed from two consecutive images with different layers of software added. The `deps` image
contains all build dependencies such as compilers, CMake, and git as well as the two main dependencies of the framework are
ROOT6 and Geant4. It derives from the latest Ubuntu LTS Docker image and can be build using the `etc/docker/Dockerfile.deps`
file via the following commands:

```shell
docker build --file etc/docker/Dockerfile.deps            \
             --tag gitlab-registry.cern.ch/allpix-squared/\
             allpix-squared/allpix-squared-deps:vX        \
             .
docker push gitlab-registry.cern.ch/allpix-squared/\
            allpix-squared/allpix-squared-deps
```

This image is created manually and only updated when necessary, i.e. if major new version of the underlying dependencies are
available. The placeholder `vX` is a version number which should be incremented when applying changes or updating software
versions in the `deps` Docker image. This version number should subsequently also be adjusted in the CI pipeline
(`.gitlab-ci.yml`) and the Allpix Squared Docker file (`/etc/docker/Dockerfile`) so that the correct dependencies image is
picked up.

{{% alert title="Important" color="warning" %}}
The Docker image containing the dependencies should not be flattened with commands like
```shell
docker export <container id> | docker import - <tag name>
```
because it strips any `ENV` variables set or used during the build process. They are used to set up the ROOT6 and Geant4
environments. When flattening, their executables and data paths cannot be found in the final Allpix Squared image.
{{% /alert %}}

Finally, the latest revision of Allpix Squared is built using the file `etc/docker/Dockerfile`. This job is performed
automatically by the continuous integration and the created containers are directly uploaded to the project's Docker
registry:

```shell
docker build --file etc/docker/Dockerfile                                \
             --tag gitlab-registry.cern.ch/allpix-squared/allpix-squared \
             .
```

A short summary of potential use cases for Docker images is provided in
[Section 2.7](../02_installation/07_docker_images.md).


[@ap2-container-registry]: https://gitlab.cern.ch/allpix-squared/allpix-squared/container_registry
