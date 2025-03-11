---
# SPDX-FileCopyrightText: 2024-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0 OR MIT
title: "NetlistWriter"
description: "A module to generate netlists for SPICE simulators"
module_status: "Immature"
module_maintainers: ["Elio Sacchetti (<elio.sacchetti@iphc.cnrs.fr>)", "Simon Spannagel (<simon.spannagel@cern.ch>)"]
module_inputs: ["PixelCharge"]
---

## Description

Integrates micro-electronics simulation elements in the Allpix Squared simulation flow. Allows the user to generate netlists (input file used by an electrical simulator to simulate the behavior of the circuit) from a given netlist template. `SPECTRE` (Cadence environment) and `SPICE` syntaxes are allowed and can be selected using the `target` parameter. This module is mostly intended for analog front-end electrical simulation using the `PixelCharge` object data.

The netlist template needs to be formatted as described and illustrated (`SPECTRE` syntax) below:

* The netlist header.
* A sub-circuit describing the circuit of interest (analog front-end for example).
* If necessary, other instances (for example other voltage or current sources of the front-end).
* A current source, which will be used to replicate the electrical behavior of the collection electrode. A particular attention should be given to the polarity of the source.
* The sub-circuit written as an instance, connected to the source.
* The netlist footer and the simulator options.

```shell
--- netlist header ---

subckt front_end Pix_in Comp_vout Comp_vref SUB VDDA VSSA Vfbk

    description of the circuit 'front_end'

ends front_end

V1 (Comp_vref 0) vsource dc=Comp_vref type=dc
V2 (SUB 0) vsource dc=0 type=dc
V3 (Vfbk 0) vsource dc=Vfbk type=dc
V4 (VDDA 0) vsource dc=1.8 type=dc

Instance_source (0 Pix_in) isource type=pulse
Instance_front_end (Pix_in Comp_vout Comp_vref SUB VDDA VSSA Vfbk) front_end

--- netlist footer and simulator options ---
```

One way to get a netlist already formatted could be to extract it from the Cadence Virtuoso environment ('schematic' view).

A new netlist is written for each event, reusing the header, footer, and circuit description from the netlist template specified with the `netlist_template` parameter. For each fired pixel, a source / circuit instance pair is added to the template.

The new source written can be parameterized with the parameter `source_type`. Two different types of sources can be used: `isource` and `isource_pulse`:

* `isource` allows writing all the temporal current waveform using a PWL (Piecewise Linear). This requires the use of the `[PulseTransfer]` module to get the current waveform. A delay can also be added using `t_delay`
* In order to lightweight the generated netlists, the `isource_pulse` can be selected: it uses the total collected charge Q (instead of the current pulse). Charge and current are linked by $ Q = \int I(t)dt $. The current pulse is set with the parameters `t_delay`, `t_rise`, `t_width` and `t_fall`. The following equation is then used to determine the current: $ I=\frac{Q}{\frac{t_{rise}+t_{fall}}{2}+t_{width}} $

<!--
The other possible source type is the `vsource_pulse`: using the total collected charge Q and the collecting electrode capacitance C, the voltage $U={Q}/{C}$ is written in the netlist.
-->

The generated netlist file name, set with the prefix `file_name` contains the number of the event.

The pixel address is used to identify the fired pixels in the netlist. If we consider the pixel (6,3) fired (7th column and 4th row) in a 10x10 matrix, the index 63 will be written in the netlist for this pixel (linearization of the 2D x and y indexes).

The parameter `waveform_to_save` is used to write at the end of the generated netlist the waveform(s) to be saved (always using the index notation to identify the fired pixels).

The electrical circuit simulation can be performed within the Allpix Squared event using the boolean parameter `run_netlist_sim` (default to `False`). If performed, the electrical simulation puts in stand-by the execution of the event.
The simulator command to execute must be given using the parameter `simulator_command`. The generated netlist name to execute is appended at the end of the command, as illustrated below for `SPECTRE` syntax:

```shell
spectre -f nutascii <file_name_event1.scs>
```

This electrical simulation is performed in the same terminal as the Allpix event, thus requiring the electrical simulator environment to be correctly set.


## Parameters

* `target`: Syntax for the additional data to be written in the netlist, either `SPECTRE` or `SPICE`.
* `netlist_template`: Location of file containing the netlist template of the circuit in one of the supported formats.
* `file_name` : Generated netlist prefix name (the suffix is the event number).
* `source_type`: Type of current source to be used, `isource` and `isource_pulse`.
* `source_name`: Name of the current source instance in the netlist.
* `subckt_name`: Name of the circuit the source is connected to.
* `common_nets`: Nets shared between the pixels.
* `t_delay`: delay from 0 before the current pulse starts, default to 0 ns
* `t_rise`: rise time of the current pulse, default to 1 ns, only works for the `isource_pulse`
* `t_width`: width of the current pulse, default to 3 ns, only works for the `isource_pulse`
* `t_fall`: fall time of the current pulse, default to 1 ns, only works for the `isource_pulse`
* `waveform_to_save`: Name of the waveforms to save
* `run_netlist_sim`: Boolean flag to select whether running the circuit simulation or not, default to false. The simulator (either `SPECTRE` or `SPICE`) environment must be loaded to run the circuit simulation in the event.
* `simulator_command`: Command to be exuted in the terminal, the generated netlist name is appended at the end of the command


## Usage

A possible configuration is using a `isource` and the `SPICE` syntax, requiring the collecting electrode capacitance:

```ini
[NetlistWriter]
target = SPICE
netlist_template = "front_end.asc"
source_type = isource
source_name = Instance_pulse
subckt_name = Instance_front_end
common_nets = Comp_vref, SUB, VDDA, VSSA, Vfbk
waveform_to_save = Pix_in, CSA_out, Comp_vout
run_netlist_sim = 1
simulator_command = "wine 'your\path\LTSpiceXVII\XVIIx64.exe' -run"
```

A current pulse `isource_pulse` and the `SPECTRE` syntax is used is this example:

```ini
[NetlistWriter]
target = SPECTRE
netlist_template = "front_end.scs"
source_type = isource_pulse
t_delay = 200ns
t_rise = 5ns
t_width = 20ns
t_fall = 5ns
source_name = Instance_pulse
subckt_name = Instance_front_end
common_nets = Comp_vref, SUB, VDDA, VSSA, Vfbk
waveform_to_save = Comp_vout
run_netlist_sim = 1
simulator_command = "spectre +aps -warn -info -log -debug -f nutascii"
```
