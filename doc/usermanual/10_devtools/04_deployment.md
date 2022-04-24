---
title: "Automatic Deployment"
weight: 4
---

The CI is configured to automatically deploy new versions of Allpix Squared and its user manual and code reference to
different places to make them available to users. This section briefly describes the different deployment end-points
currently configured and in use. The individual targets are triggered either by automatic nightly builds or by publishing new
tags. In order to prevent accidental publications, the creation of tags is protected. Only users with Maintainer privileges
can push new tags to the repository. For new tagged versions, all deployment targets are executed.

### Software deployment to CVMFS

The software is automatically deployed to CERN's VM file system (CVMFS) \[[@cvmfs]\] for every new tag. In addition, the
`master` branch is built and deployed every night. New versions are published to the folder
`/cvmfs/clicdp.cern.ch/software/allpix-squared/` where a new folder is created for every new tag, while updates via the
`master` branch are always stored in the `latest` folder.

The deployed version currently comprises all modules as well as the detector models shipped with the framework. An additional
`setup.sh` is placed in the root folder of the respective release, which allows to set up all runtime dependencies necessary
for executing this version. Versions both for CentOS 7 and CentOS 8 are provided.

The deployment CI job runs on a dedicated computer with a GitLab SSH runner. Job artifacts from the packaging stage of the CI
are downloaded via their ID using the script found in `.gitlab/ci/download_artifacts.py`, and are made available to the
`cvclicdp` user which has access to the CVMFS interface. The job checks for concurrent deployments to CVMFS and then unpacks
the tarball releases and publishes them to the CLICdp experiment CVMFS space, the corresponding script for the deployment can
be found in `.gitlab/ci/gitlab_deployment.sh`. This job requires a private API token to be set as secret project variable
through the GitLab interface, currently this token belongs to the service account user `ap2`.

### Documentation deployment to EOS

The project documentation is deployed to the project's EOS space at `/eos/project/a/allpix-squared/www/` for publication on
the project website. This comprises both the PDF and HTML versions of the user manual (subdirectory `usermanual`) as well as
the Doxygen code reference (subdirectory `reference/`). The documentation is only published for new tagged versions of the
framework.

The CI jobs uses the `ci-web-deployer` Docker image from the CERN GitLab CI tools to access EOS, which requires a specific
file structure of the artifact. All files in the artifact's `public/` folder will be published to the `www/` folder of the
given project. This job requires the secret project variables `EOS_ACCOUNT_USERNAME` and `EOS_ACCOUNT_PASSWORD` to be set via
the GitLab web interface. Currently, this uses the credentials of the service account user `ap2`.

### Release tarball deployment to EOS

Binary release tarballs are deployed to EOS to serve as downloads from the website to the directory
`/eos/project/a/allpix-squared/www/releases`. New tarballs are produced for every tag as well as for nightly builds of the
`master` branch, which are deployed with the name `allpix-squared-latest-<system-tag>-opt.tar.gz`.

The files are taken from the packaging jobs and published via the `ci-web-deployer` Docker image from the CERN GitLab CI
tools. This job requires the secret project variables `EOS_ACCOUNT_USERNAME` and `EOS_ACCOUNT_PASSWORD` to be set via the
GitLab web interface. Currently, this uses the credentials of the service account user `ap2`.


[@cvmfs]: https://pos.sissa.it/070/052/
