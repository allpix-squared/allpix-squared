---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "DepositionLaser"
description: "A simplistic deposition generator for charge injection with a laser. Mainly intended for TCT studies simulations."
module_maintainer: "Daniil Rastorguev (<daniil.rastorguev@desy.de>)"
module_status: "Immature"
# module_output: "DepositedCharge, MCParticle"
---

## Description

Beam: cylindric beam with gaussian profile
TODOs:
* Converging/diverging beam
* Pulse shape in config
* Beam waist in waist units instead of sigma

Photon tracking: photons are currently tracked with straight lines using Liang-Barsky clipping, exponential (w.r.t. depth) absorption in silicon sensors is generated
TODOs:
* Terminate tracks if a passive object is hit 
* Photon refraction on sensor surface

Lookup table for absorption from [[1]](#1) is used





## Parameters
* `photon_number`: number of photons in a *single* event, defaults to 10000
* `source_position`
* `beam_direction`
* `wavelength` in *nm*
* `beam_waist`: std_dev of transversal beam profile, defaults to 20 um
* `verbose_tracking`: defaults to `false`. If set to `true`, it will increase amount of tracking-related debug log output (and may slightly increase computing cost if multiple detectors are present)

## Usage
*Example how to use this module*

## References
<a id="1">[1]</a> 
M. A. Green and Keevers, M. J., “Optical properties of intrinsic silicon at 300 K”, 
Progress in Photovoltaics: Research and Applications, vol. 3, pp. 189 - 192, 1995.
