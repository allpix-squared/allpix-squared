---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
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
for executing this version. Versions both for CentOS 7 and CentOS 8 are provided.

The deployment CI job runs on a dedicated computer with a GitLab SSH runner. Job artifacts from the packaging stage of the CI
are downloaded via their ID using the script found in `.gitlab/ci/download_artifacts.py`, and are made available to the
`cvclicdp` user which has access to the CVMFS interface. The job checks for concurrent deployments to CVMFS and then unpacks
the tarball releases and publishes them to the CLICdp experiment CVMFS space, the corresponding script for the deployment can
be found in `.gitlab/ci/gitlab_deployment.sh`. This job requires a private API token to be set as secret project variable
through the GitLab interface, currently this token belongs to the service account user `ap2`.

### Documentation deployment

The documentation is provided as an online version and a PDF. Both get generated from the Markdown \[[@markdown]\]
documentation found in the project repository \[[@ap2-repo]\]. For the PDF the plain Markdown documentation is converted via
pandoc \[[@pandoc]\] and a Python script adjusting to the LaTeX for the PDF.

The CI deploys the PDF on CERN's EOS at `/eos/project/a/allpix-squared/www/usermanual/`. The PDF documentation is published
for new tagged versions of the framework and for nightlies (version `latest`). The version number is attached to the file
name so that the [website](https://project-allpix-squared.web.cern.ch/usermanual/) contains the usermanual for all versions.

The CI jobs uses the `ci-web-deployer` Docker image from the CERN GitLab CI tools to access EOS, which requires a specific
file structure of the artifact. All files in the artifact's `public/` folder will be published to the `www/` folder of the
given project. This job requires the secret project variables `EOS_ACCOUNT_USERNAME` and `EOS_ACCOUNT_PASSWORD` to be set via
the GitLab web interface. Currently, this uses the credentials of the service account user `ap2`.

The online version of the documentation is included in the project's website, which uses hugo \[[@hugo]\] to generate HTML
from Markdown. The project website is hosted in its own repositroy \[[@ap2-website-repo]\] and deployed directly from the CI
via [GitLab Pages](https://docs.gitlab.com/ee/user/project/pages/) (see also
[how-to.docs.cern.ch](https://how-to.docs.cern.ch/)).

If a new tag is pushed to the project repository, the CI in the project repository triggers a CI pipeline in the website
repository. This pipeline clones the project repository to get the Markdown documentation, generates the HTML with hugo and
deploys it. This setup allows to update information on the website without pushing a new tag to the project repository.

For more information on the documentation, see `doc/README.md` in the project repository \[[@ap2-repo]\].


[@cvmfs]: https://pos.sissa.it/070/052/
[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
[@markdown]: https://docs.gitlab.com/ee/user/markdown.html
[@ap2-website-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared-website/
[@hugo]: https://gohugo.io/
