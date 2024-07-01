---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Coding and Naming Conventions"
weight: 1
---

The code base of the Allpix Squared is well-documented and follows concise rules on naming schemes and coding conventions.
This enables maintaining a high quality of code and ensures maintainability over a longer period of time. In the following,
some of the most important conventions are described. In case of doubt, existing code should be used to infer the coding
style from.

## Naming Schemes

The following coding and naming conventions should be adhered to when writing code which eventually should be merged into the
main repository.

* **Namespace**:
  The `allpix` namespace is to be used for all classes which are part of the framework, nested namespaces may be defined.
  It is encouraged to make use of `using namespace allpix;` in implementation files only for this namespace. Especially the
  namespace `std` should always be referred to directly at the function to be called, e.g. `std::string test`. In a few
  other cases, such as `ROOT::Math`, the `using` directive may be used to improve readability of the code.

* **Class names**:
  Class names are typeset in CamelCase, starting with a capital letter, e.g. `class ModuleManager`. Every class should
  provide sensible Doxygen documentation for the class itself as well as for all member functions.

* **Member functions**:
  Naming conventions are different for public and private class members. Public member function names are typeset as
  camelCase names without underscores, e.g. `getElectricFieldType()`. Private member functions use lower-case names,
  separating individual words by an underscore, e.g. `create_detector_modules(...)`. This allows to visually distinguish
  between public and restricted access when reading code.

  In general, public member function names should follow the `get`/`set` convention, i.e. functions which retrieve
  information and alter the state of an object should be marked accordingly. Getter functions should be made `const` where
  possible to allow usage of constant objects of the respective class.

* **Member variables**:
  Member variables of classes should always be private and accessed only via respective public member functions. This
  allows to change the class implementation and its internal members without requiring to rewrite code which accesses them.
  Member names should be typeset in lower-case letters, a trailing underscore is used to mark them as member variables,
  e.g. `bool terminate_`. This immediately sets them apart from local variables declared within a function.

## Formatting


A set of formatting rules is applied to the code base in order to avoid unnecessary changes from different editors and to
maintain readable code. It is vital to follow these rules during development in order to avoid additional changes to the
code, just to adhere to the formatting. There are several options to integrate this into the development workflow:

* Many popular editors feature direct integration either with `clang-format` or their own formatting facilities.

* A build target called `make format` is provided if the `clang-format` tool is installed. Running this command before
  committing code will ensure correct formatting.

* This can be further simplified by installing the *git hook* provided in the directory `/etc/git-hooks/`. A hook is a
  script called by `git` before a certain action. In this case, it is a pre-commit hook which automatically runs
  `clang-format` in the background and offers to update the formatting of the code to be committed. It can be installed
  by calling

  ```shell
  ./etc/git-hooks/install-hooks.sh
  ```

  once.

The formatting rules are defined in the `.clang-format` file in the repository in machine-readable form (for `clang-format`,
that is) but can be summarized as follows:

* The column width should be 125 characters, with a line break afterwards.

* New scopes are indented by four whitespaces, no tab characters are to be used.

* Namespaces are indented just as other code is.

* No spaces should be introduced before parentheses `()`.

* Included header files should be sorted alphabetically.

* The pointer asterisk should be left-aligned, i.e. `int* foo` instead of `int *foo`.

The continuous integration automatically checks if the code adheres to the defined format as described in
[Section 11.3](../11_devtools/03_ci.md).
