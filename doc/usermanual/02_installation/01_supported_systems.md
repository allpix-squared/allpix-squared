---
# SPDX-FileCopyrightText: 2022-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Supported Operating Systems and Compilers"
weight: 1
---

## Operating Systems

Allpix Squared is designed to run without issues on either a recent Linux distribution or Mac OS X. Furthermore, the
continuous integration of the project ensures correct building and functioning of the software framework on CentOS 7 (with
GCC), AlmaLinux 9 and Red Hat Enterprise Linux 9 (with GCC and LLVM), Ubuntu 22.04 LTS (Docker, GCC) and Mac OS Catalina 10.15 (AppleClang 12.0).

## Compilers

Allpix Squared relies on functionality from the C++17 standard and therefore requires a C++17-compliant compiler. This
comprises for example GCC 9+, LLVM/Clang 4.0+ and AppleClang 10.0+. A detailed list of supported compilers can be found at
\[[@cppcompilersupport]\].


[@cppcompilersupport]: https://en.cppreference.com/w/cpp/compiler_support/17
