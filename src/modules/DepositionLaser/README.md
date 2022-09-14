---
# SPDX-FileCopyrightText: 2017-2022 CERN and the Allpix Squared authors
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
* Wavelength and pulse shape in config
* Beam waist in waist units instead of sigma

Photon tracking: photons are currently tracked with straight lines using Liang-Barsky clipping, exponential (w.r.t. depth) absorption in silicon sensors is generated
TODOs:
* Terminate tracks if a passive object is hit 
* Lookup table with absorption coefficients for different wavelengths
* Photon refraction on sensor surfaces





## Parameters
* `source_position`
* `beam_direction`
* `photon_number`: number of photons in a *single* event, defaults to 10000
* `beam_waist`: std_dev of transversal beam profile, defaults to  20 um

## Usage
*Example how to use this module*
