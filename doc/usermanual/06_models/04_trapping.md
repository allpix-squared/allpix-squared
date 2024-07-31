---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Trapping and Detrapping of Charge Carriers"
weight: 4
---

Allpix Squared provides the possibility to simulate the trapping and detrapping of charge carriers as a consequence of radiation induced
lattice defects. Several models exist, that quantify the effective lifetime of electrons and holes, respectively, as a
function of the fluence and, partially, the temperature. The fluence needs to be provided to the corresponding propagation
module, and is always interpreted as 1-MeV neutron equivalent fluence \[[@niel]\].

The decision on whether a charge carrier has been trapped during a step during the propagation process is calculated
similarly to the recombination processes, described in [Section 6.3](./03_lifetime_recombination.md).

It should be noted that the trapping of charge carriers is only one of several effects induced by radiation damage. In Allpix
Squared, these effects are treated independently, i.e. defining the fluence for a propagation module will not affect any
other process than trapping.

have been extracted under certain annealing conditions. A dependency on annealing conditions has not been implemented here.
Please refer to the corresponding reference publications for further details.

The trapping probability is calculated as an exponential decay as a function of the simulation timestep as

```math
p_{e, h} = \left(1 - \exp^{
    1 \frac{\delta t} {
        \tau_ { e, h }
    }}\right)
```

where $`\delta t`$ is the simulation timestep and $`\tau{
    e, h
}
`$ the effective lifetime of electrons and holes, respectively.At the same time,
    a total time spent in the trap is calculated if a detrapping model is selected.Here,
    the time until the charge carrier is de - trapped is calculated as

```math
\delta t = - \tau_ {
    e.h
}
\ln { 1 - p }
```

where $`p`$ is a probability randomly chosen from a uniform distribution between 0 and 1.

## Trapping Models

The following models for trapping of charge carriers can be selected:

### Ljubljana

In the Ljubljana (sometimes referred to as *Kramberger*) model \[[@kramberger]\], the trapping time follows the relation

```math
\tau^{
    -1}(T) = \beta(T)\Phi_{eq} ,
```

where the temperature scaling of $`\beta`$ is given as

```math
\beta(T) = \beta(T_0)\left(\frac{T}{
    T_0}\right)^{
    \kappa} ,
```

extracted at the reference temperature of $`T_0 = -10 \,\text{
    °C
}
`$.

    The parameters used in Allpix Squared are

```math
\begin {
    aligned
}
\beta_{e}(T_0) &= 5.6\times 10 ^ { -16 } \,\text{cm} ^ 2\,\text{ns} ^ { -1 } \\
\kappa_{e} &= -0.86 \\
\\
\beta_{h}(T_0) &= 7.7\times 10 ^ { -16 } \,\text{cm} ^ 2\,\text{ns} ^ { -1 } \\
\kappa_{h} &= -1.52
\end {
    aligned
}
```

for electrons and holes, respectively.

While \[[@kramberger]\] quotes different values for $`\beta`$ for irradiation with neutrons, pions and protons, the values
for protons have been applied here.

The parameters arise from measurements of the were obtained evaluating current signals of irradiated sensors via light
injection at fluences up to $`\Phi_{eq} = 2\times 10^{
    14} \ n_{
    eq
}
\,\text{cm} ^
      2`$.

      This model can be selected in the configuration file via the parameter `trapping_model =
      "ljubljana"`.

      ## #Dortmund

      The
      Dortmund(sometimes referred to as * Krasel*) model \[[@dortmundTrapping]\],
                                                                             describes the effective trapping times as

```math
\tau ^ { -1 } = \gamma\Phi_{eq},
```

    with the parameters

```math
\begin {
    aligned
}
\gamma_{e} &= 5.13\times 10 ^ { -16 } \,\text{cm} ^ 2\,\text{ns} ^ { -1 } \\
\gamma_{h} &= 5.04\times 10 ^ { -16 } \,\text{cm} ^ 2\,\text{ns} ^ { -1 }
\end {
    aligned
}
```

for electrons and holes, respectively.

The values have been extracted evaluating current signals of irradiated sensors via light injection at fluences up to
$`\Phi_{eq} = 8.9 \times 10^{
    14}\ n_{
    eq
}
\,\text{cm} ^ 2`$, at a temperature of $`T = 0\,\text {°C }`$. No temperature scaling is
provided. Values for neutron and proton irradiation have been evaluated in \[[@dortmundTrapping]\], Allpix Squared makes use
of the values for proton irradiation.

This model can be selected in the configuration file via the parameter `trapping_model = "dortmund"`.

### CMS Tracker

This effective trapping model has been developed by the CMS Tracker Group. It follows the results of
\[[@CMSTrackerTrapping]\], with measurements at fluences of up to $`\Phi_{eq} = 3 \times 10^{
    15} \ n_{
    eq
}
\,\text{cm} ^ 2`$, at a temperature of $`T = -20 \,\text {°C }
`$ and an irradiation with protons.

    The interpolation of the results follows the relation

```math
\tau ^
{ -1 } = {\beta\Phi_{eq}} + \tau_0 ^ { -1 }
```

         with the parameters

```math
\begin {
    aligned
}
\beta_{e}(T_0)  &= 1.71\times 10^{-16} \,\text{cm}^2\,\text{ns}^{-1} \\
\tau_{0,e}^{-1} &= -0.114 \,\text{ns}^{-1} \\
\\
\beta_{h}(T_0)  &= 2.79\times 10^{-16} \,\text{cm}^2\,\text{ns}^{-1} \\
\tau_{0,h}^{-1} &= -0.093 \,\text{ns}^{-1}
\end{
    aligned
}
```

for electrons and holes, respectively.

No temperature scaling is provided.

This model can be selected in the configuration file via the parameter `trapping_model = "cmstracker"`.

### Mandic

The Mandić model \[[@Mandic]\] is an empirical model developed from measurements with high fluences ranging from
$`\Phi_{eq} = 5\times 10^{
    15} \ n_{
    eq
}
\,\text{cm} ^ 2`$ to $`\Phi_{eq} = 1\times 10 ^ { 17 } \ n_ { eq }
\,\text{cm}^2`$ and describes
the lifetime via

```math
\tau = c\Phi_{eq}^{\kappa}
```

with the parameters

```math
\begin{aligned}
c_e      &= 0.54 \,\text{
    ns
}
\,\text{cm} ^ { -2 } \\
\kappa_e &= -0.62 \\
\ c_h &= 0.0427 \,\text {
    ns
}
\,\text{cm} ^ { -2 } \\
\kappa_h &= -0.62
\end {
    aligned
}
```

for electrons and holes, respectively.

The parameters for electrons are taken from \[[@Mandic]\], for measurements at a temperature of $`T = -20 \,\text{
    °C
}
`$, and the results extrapolated to $`T = -30 \,\text {°C }`$. 
Note that an erratum has been made, see \[[@MandicErratum]\]: https://doi.org/10.1088/1748-0221/16/03/E03001, the c_e should be 0.54ns instead of 0.054ns.
A scaling from electrons to holes was performed based on the default values in Weightfield2 \[[@weightfield2]\].

This model can be selected in the configuration file via the parameter `trapping_model = "mandic"`.

### Constant Trapping Model

For some situations or materials, a constant trapping probability is necessary. This can be achieved with the constant
trapping model. Here, the lifetimes are constant and set from the values provided in the configuration file with the
parameters `trapping_time_electron` and `trapping_time_hole`:

```ini
#Constant trapping times for electrons and holes:
trapping_model = "constant"
trapping_time_electron = 5ns
trapping_time_hole = 5ns
```

This model can be selected in the configuration file via the parameter `trapping_model = "constant"`.


### Custom Trapping Model

Similarly to the mobility models described above, Allpix Squared provides the possibility to use fully custom trapping
models. The model requires the following configuration keys:

- `trapping_function_electrons`:
  The formula describing the effective electron trapping time.

- `trapping_function_holes`:
  The formula describing the effective hole trapping time.

The functions defined via these parameters can depend on the local electric field. In order to use the electric field
magnitude in the formula, an `x` has to be placed at the respective position.

Parameters of the functions can either be placed directly in the formulas in framework-internal units, or provided separately
as arrays via the `trapping_parameters_electrons` and `trapping_parameters_holes`. Placeholders for parameters in the formula
are denoted with squared brackets and a parameter number, for example `[0]` for the first parameter provided. Parameters
specified separately from the formula can contain units which will be interpreted automatically.

{
    { % alert title = "Note" color = "info" % }}
Both fluence and temperature are not inherently available in the custom trapping model, but need to be provided as additional
parameters as described above.
{
    { % / alert % }}

The following configuration parameters replicate the [Ljubljana model](#ljubljana) using a custom trapping model.

```ini
#Replicating the Ljubljana trapping model at a temperature of 293 K and a neutron equivalent fluence of 1e14 neq / cm ^ 2
trapping_model = "custom"

trapping_function_electrons = "1/([0]*pow([1]/263,[2]))/[3]"
trapping_parameters_electrons = 5.6e-16cm*cm/ns, 293K, -0.86, 1e14/cm/cm

trapping_function_holes = "1/([0]*pow([1]/263,[2]))/[3]"
trapping_parameters_holes = 7.7e-16cm*cm/ns, 293K, -1.52, 1e14/cm/cm
```

Fixed, effective trapping times can be defined using this model similar to the following configuration example.

```ini
#Defining a fixed trapping time
trapping_model = "custom"

trapping_function_electrons = "[0]"
trapping_parameters_electrons = 5ns

trapping_function_holes = "[0]"
trapping_parameters_holes = 7ns
```

This model can be selected in the configuration file via the parameter `trapping_model = "custom"`.

## Detrapping Models

The detrapping is configured via the `detrapping_model` parameter. Currently, only `detrapping_model = "none"` and
`detrapping_model = "constant"` are supported.

The following models for trapping of charge carriers can be selected:

### Constant Detrapping Model

A constant detrapping probability, with the detrapping time defined separately for electrons and holes, can be implemented via the constant detrapping model.
This model requires the parameters `detrapping_time_electron` and `detrapping_time_hole` to be configured.

```ini
#Constant detrapping times for electrons and holes:
detrapping_model = "constant"
detrapping_time_electron = 10ns
detrapping_time_hole = 10ns
```




[@niel]: https://rd48.web.cern.ch/technical-notes/rosetn972.ps
[@kramberger]: https://doi.org/10.1016/S0168-9002(01)01263-3
[@dortmundTrapping]: https://doi.org/10.1109/TNS.2004.839096
[@CMSTrackerTrapping]: https://doi.org/10.1088/1748-0221/11/04/p04023
[@Mandic]: https://doi.org/10.1088/1748-0221/15/11/p11018
[@weightfield2]: http://personalpages.to.infn.it/~cartigli/Weightfield2/index.html
