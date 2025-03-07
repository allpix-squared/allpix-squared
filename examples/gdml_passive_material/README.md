---
# SPDX-FileCopyrightText: 2021-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "GDML Passive Material"
description: "Example loading passive materials from GDML files"
---

This example demonstrates how to load passive material structures defined in GDML files into the simulation.
Two separate GDML files are placed via the detector setup description file and loaded at startup.
All contained volumes are added to the simulation

The setup consists of two silicon detectors and the additional volumes from the GDML files.
Some of the volumes are placed between the particle beam origin and the detectors in order to have the pions interact with the material.
