---
title: "Building Docker Images"
weight: 5
---

New Allpix Squared Docker images are automatically created and deployed
by the CI for every new tag and as a nightly build from the `master`
branch. New versions are published to project Docker container
registry \[[@ap2-container-registry]\]. Tagged versions can be found
via their respective tag name, while updates via the nightly build are
always stored with the `latest` tag attached.

The final Docker image is formed from two consecutive images with
different layers of software added. The `deps` image contains all build
dependencies such as compilers, CMake, and git as well as the two main
dependencies of the framework are ROOT6 and Geant4. It derives from the
latest Ubuntu LTS Docker image and can be build using the
`etc/docker/Dockerfile.deps` file via the following commands:
```sh
docker build --file etc/docker/Dockerfile.deps            \
             --tag gitlab-registry.cern.ch/allpix-squared/\
             allpix-squared/allpix-squared-deps           \
             .
docker push gitlab-registry.cern.ch/allpix-squared/\
            allpix-squared/allpix-squared-deps
```

This image is created manually and only updated when necessary, i.e. if
major new version of the underlying dependencies are available.

{{% alert title="Important" color="warning" %}}
The dependencies Docker image should not be flattened with commands like
```sh
docker export <container id> | docker import - <tag name>
```
because it strips any `ENV` variables set or used during the build
process. They are used to set up the ROOT6 and Geant4 environments. When
flattening, their executables and data paths cannot be found in the
final Allpix Squared image.
{{% /alert %}}

Finally, the latest revision of Allpix Squared is built using the file
`etc/docker/Dockerfile`. This job is performed automatically by the
continuous integration and the created containers are directly uploaded
to the project's Docker registry.
```sh
docker build --file etc/docker/Dockerfile                                \
             --tag gitlab-registry.cern.ch/allpix-squared/allpix-squared \
             .
```

A short summary of potential use cases for Docker images is provided in
the Installation chapter.


[@ap2-container-registry]: https://gitlab.cern.ch/allpix-squared/allpix-squared/container_registry
