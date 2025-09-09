---
# SPDX-FileCopyrightText: 2017-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "How to contribute"
---

Thanks for considering to contribute to Allpix Squared. Any type of merge request, ranging from small bugfixes, improvements
to the documentation to entirely new functionality, is much appreciated. We, the maintainers, will try to our best to look
carefully at every merge request.

If you only want to submit an issue, that is also welcome, please continue directly to the issue tracker
\[[@ap2-issue-tracker]\] to open a ticket.

The following is a set of guidelines that will help both you as submitter as well as us maintainers to make it as easy as
possible to contribute changes.

## Core and modules

Allpix Squared is split up in a slim core, providing base functionality as configuration, detector geometry, management of
modules, messaging as well as various utilities. The actual chain of simulation is developed in independent modules. Please
try to separate any merge request for improvements or changes to the core from the implementation and updates of modules.
Generally any kind of separable simulation functionality should be implemented in its own module and submitted as a
individual merge request. Also try to submit an individual merge request for each independent change to allow us to review
them separately.

If you have any doubt about the best way to implement new functionality or how to split it up, please open an issue with the
discussion tag on the issue tracker \[[@ap2-issue-tracker]\]. Also please do open an incomplete merge request as
soon as possible with the "Draft" label to allow for early discussion.

## Getting started

Please follow the next steps to setup your system for contributing. Note that these are slightly different from the normal
installation instructions.

1. Fork the repository by clicking on "Fork" on the main repository \[[@ap2-repo]\].
2. Clone your local fork using `git clone https://gitlab.cern.ch/your-username/allpix-squared` (when using HTTPS, this has
   to be changed accordingly for SSH or KRB5)
3. Install the latest version of the *clang* package with the *clang-format* and *clang-tidy* programs.
4. Follow the build instructions using CMake explained in the User's manual.

If you don't have an account for [CERN's GitLab instance](https://gitlab.cern.ch) (restricted to CERN associates), you can
fork the [GitHub repository](https://github.com/allpix-squared/allpix-squared) instead.

## Making changes

Now you can start making changes and adding new functionality to the code.

1. Run `pre-commit install -f` from the repository top folder to install the git hooks that automatically update
   the format of the code to comply with the coding style.
2. Create a new branch from master with a description of the change using `git checkout -b my-new-branch-name`.
3. Read the relevant sections in the User's manual before starting to make changes.
4. Implement the new code and frequently commit using `git commit -m 'my commit message'`. Please use descriptive messages
   explaining what changed.
5. Push the code to your local mirror using `git push --set-upstream origin`.
6. Retrieve the latest changes to the upstream master every now and then. To do this add the upstream version to your
   remotes using `git remote add upstream https://gitlab.cern.ch/allpix-squared/allpix-squared.git` (or the SSH or KRB5
   version if preferred). This only has to be done once, the first time after cloning the repository. Afterwards you fetch
   the changes using `git fetch upstream`. Then you can add the change preferably using rebase with
   `git rebase upstream/master`. If that causes problems you can use merge with `git merge upstream/master`.

## Submitting a merge request

As soon as there exists something in your branch, a merge request can be opened on the main repository. If you don't have an
account for [CERN's GitLab instance](https://gitlab.cern.ch) (restricted to CERN associates), you should instead open a pull
request as explained below. Do not forget that it is not a problem to open a merge request for incomplete implementations.

1. Retrieve the latest changes from the upstream version as explained above.
2. Optionally format the code if you did not add the git-hook from the beginning, this can be done manually by running
   `make format` from the build directory.
3. Go to [merge request](https://gitlab.cern.ch/allpix-squared/allpix-squared/merge_requests) and click on "New merge
   request".
4. Follow the instructions. Do not forget to use the 'Draft:' prefix if your code is only partially ready. Then submit the
   merge request.
5. Please wait for the maintainers to give you access to the continuous integration (CI) runners that will check your code
   if you do not already have it.
6. Add all the specific runners on your local repository at
   https://gitlab.cern.ch/your-username/allpix-squared/settings/ci_cd.
7. The pipeline can now be restarted and the CI will check your changes. If the CI fails and gives an error please refer to
   the log containing a description about what went wrong. It is likely that errors will appear because Allpix Squared
   enforces a strict policy of compiler errors and requires full compliance of the clang-tidy "linter" tool, which
   frequently complains about minor changes (it might help to search for `error:` to find the actual error(s) in the
   output). This clang-tidy tool can also be run locally on your pc by executing `make check-lint` from the build directory.
   Easy changes can be fixed automatically by executing `make lint`.
8. The maintainers will look at your proposed changes and likely provide some (constructive) feedback.
9. Please continue to update the code with the received comments until every reviewer and the continuous integration is
   happy :)
10. Your merge request can now be merged in. Congratulations and thank you so much, you have contributed something new to
    the repository.

## Submitting a pull request

As soon as there exists something in your branch, a pull request can be opened on the main repository. Do not forget that it
is not a problem to open a pull request marked as draft for incomplete implementations.

1. Retrieve the latest changes from the upstream version as explained above.
2. Optionally format the code if you did not add the git-hook from the beginning, this can be done manually by running
   `make format` from the build directory.
3. Go to [pull request](https://github.com/allpix-squared/allpix-squared/pulls) and click on "New pull request". Select
   "compare across forks" and choose your fork and the appropriate branch on your fork. Then click "Create pull request".
4. Follow the instructions. Do not forget to select "Create draft pull request" if your code is only partially ready.
   Then submit the pull request.
5. GitHub actions will automatically check your changes. If GitHub actions yields an error please refer to the log containing
   a description about what went wrong. It is likely that errors will appear because Allpix Squared enforces a strict policy
   of compiler errors and requires full compliance of the clang-tidy "linter" tool, which frequently complains about minor
   changes (it might help to search for `error:` to find the actual error(s) in the output). This clang-tidy tool can also
   be run locally on your pc by executing `make check-lint` from the build directory. Easy changes can be fixed
   automatically by executing `make lint`.
6. The maintainers will look at your proposed changes and likely provide some (constructive) feedback.
7. Please continue to update the code with the received comments until every reviewer and the continuous integration is
   happy :)
8. Your pull request can now be merged. Congratulations and thank you so much, you have contributed something new to the
   repository.


[@ap2-issue-tracker]: https://gitlab.cern.ch/allpix-squared/allpix-squared/issues
[@ap2-repo]: https://gitlab.cern.ch/allpix-squared/allpix-squared
