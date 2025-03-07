---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Charge Carrier Lifetime & Recombination"
weight: 3
---

Allpix Squared provides the possibility to simulate finite lifetimes of charge carriers as a function of the local doping
concentration via non-radiative recombination processes. While most of these models require the *total doping concentration*
$`N_D + N_A`$ as parameter, the doping profile used throughout Allpix Squared provides the *effective doping concentration*
$`N_D - N_A`$ since this also encodes the majority charge carriers via its sign - an information relevant to some of the
models. However, in the parts of a silicon detector relevant for this simulation, i.e. the sensing volume, the difference
between effective and total concentration is expected to be negligible. Therefore the two values are treated as equivalent
throughout the lifetime models and the doping concentration is taken as the absolute value $`N = \left|N_D - N_A\right|`$.

Whether a charge carrier has recombined with the lattice is calculated for every step of the simulation using the relation

```math
p < 1 - e^{- dt / \tau(N)}
```

where $`p`$ is a recombination probability, drawn from a uniform distribution with $`[0, 1]`$, $`dt`$ is the last time step
of the charge carrier motion and $`\tau`$ the lifetime for the local doping concentration calculated by the models described
in the following. If the above equation evaluates to *false*, the charge carrier still exists, if it evaluates to *true* it
has been recombined with the lattice.

Finite charge carrier lifetime can be simulated by all propagation modules and comprise the following models:

## Shockley-Read-Hall Recombination

This model describes the finite lifetime based on Shockley-Read-Hall or trap-assisted recombination of charge carriers with
the lattice \[[@shockley-read], [@hall]\]. The lifetime is calculated using the Shockley-Read-Hall relation as given by
\[[@fossum-lee]\]:

```math
\tau(N) = \frac{\tau_0}{1 + \frac{N}{N_{d0}}}
```

where $`\tau_0`$ and $`N_{d0}`$ are reference lifetime and doping concentration, for electrons and holes respectively. The
parameter values implemented in Allpix Squared are taken from \[[@fossum-lee]\] and the Synopsys Sentaurus TCAD software
manual as

```math
\begin{aligned}
\tau_{0,e} &=  1\times 10^{-5} \,\text{s} \\
N_{d0,e}   &=  1\times 10^{16} \,\text{cm}^{-3} \\
\\
\tau_{0,h} &= 4.0\times 10^{-4} \,\text{s} \\
N_{d0,h}   &= 7.1\times 10^{15} \,\text{cm}^{-3}
\end{aligned}
```

for electrons and holes, respectively.

The temperature dependence of the Shockley-Read-Hall lifetime is scaled following the low-temperature approximation model
presented \[[@schenk]\] as:

```math
\tau(N, T) = \tau(N) \cdot \left( \frac{300 \,\text{K}}{T} \right)^{3/2}
```

This model can be selected in the configuration file via the parameter `recombination_model = "srh"`.

## Auger Recombination

At high doping levels exceeding $`5\times 10^{18} \,\text{cm}^{-3}`$ \[[@fossum-lee2]\], the Auger recombination model becomes
increasingly important. It assumes that the excess energy created by electron-hole recombinations is transferred to another
electron (*e-e-h process*) or another hole (*e-h-h process*). The total recombination rate is then given by \[[@kerr]\]:

```math
R_{Auger} = C_n n^2p + C_p n p^2
```

where $`C_n`$ and $`C_p`$ are the Auger coefficients. The first term corresponds to the e-e-h process and the second term to
the e-h-h process. In highly-doped silicon, the Auger lifetime for minority charge carriers can be written as:

```math
\tau(N) = \frac{1}{C_{a} \cdot N^2}
```

where $`C_{a} = C_{n} + C_{p}`$ is the ambipolar Auger coefficient, taken as
$`C_{a} = 3.8\times 10^{-31} \,\text{cm}^6\,\text{s}^{-1}`$ from \[[@dziewior]\].

This recombination mode applies to minority charge carriers only, majority charge carriers have an infinite life time under
this model and the recombination equation will always evaluate to *true*.

This model can be selected in the configuration file via the parameter `recombination_model = "auger"`.

## Combined SRH/Auger Recombination

This model combines the charge carrier recombination from the Shockley-Read-Hall and the Auger model by inversely summing the
individual lifetimes calculated by the models via

```math
\begin{aligned}
\tau^{-1}(N) &= \tau_{srh}^{-1}(N) + \tau_{a}^{-1}(N) &\quad \text{for minority charge carriers} \\
             &= \tau_{srh}^{-1}(N)                    &\quad \text{for majority charge carriers}
\end{aligned}
```

where $`\tau_{srh}(N)`$ is the Shockley-Read-Hall and $`\tau_{a}(N)`$ the Auger lifetime. The latter is only taken into
account for minority charge carriers.

This model can be selected in the configuration file via the parameter `recombination_model = "srh_auger"`.

## Recombination with Constant Lifetimes

Some materials require constant lifetimes for charge carriers $`\tau(N) = \tau`$. This model requires the additional
configuration keys `lifetime_electron` and `lifetime_hole` to be present in the module configuration section, for example:

```ini
# Constant lifetimes for electrons and holes in GaAs with Cr compensation:
recombination_model = "constant"
lifetime_electron = 30ns
lifetime_hole = 4.5ns
```

This model can be selected in the configuration file via the parameter `recombination_model = "constant"`.

## Custom Recombination Models

Allpix Squared provides the possibility to use fully custom recombination models. In order to use a custom model, the parameter
`recombination_model = "custom"` needs to be set in the configuration file. Additionally, the following configuration keys have to
be provided:

- `lifetime_function_electrons`:
  The formula describing the electron lifetime.

- `lifetime_function_holes`:
  The formula describing the hole lifetime.

The functions defined via these parameters can depend on the local doping concentration. In
order to use the doping concentration in the formula, an `x` has to be placed at the respective position.

Parameters of the functions can either be placed directly in the formulas in framework-internal units, or provided separately
as arrays via the `lifetime_parameters_electrons` and `lifetime_parameters_electrons`. Placeholders for parameters in the
formula are denoted with squared brackets and a parameter number, for example `[0]` for the first parameter provided.
Parameters specified separately from the formula can contain units which will be interpreted automatically.

{{% alert title="Warning" color="warning" %}}
Parameters directly placed in the recombination formula have to be supplied in framework-internal units since the function will be
evaluated with the doping concentration in internal units. It is recommended to use the
possibility of separately configuring the parameters and to make use of units to avoid conversion mistakes.
{{% /alert %}}

The following set of parameters re-implements the [Shockley-Read-Hall recombination](#shockley-read-hall-recombination) model using a custom
recombination model. The lifetimes are calculated at a fixed temperature of 293 Kelvin.

```ini
# Replicating the Shockley-Read-Hall model at T = 293K
recombination_model = "custom"

lifetime_function_electrons = "[0]/(1 + x / [1])"
lifetime_parameters_electrons = 1.036e-5s, 1e16/cm/cm/cm

lifetime_function_holes = "[0]/(1 + x / [1])"
lifetime_parameters_holes = 4.144e-4s, 7.1e15/cm/cm/cm
```

{{% alert title="Warning" color="warning" %}}
It should be noted that the temperature passed via the module configuration is not evaluated for the custom recombination model,
but the model parameters need to be manually adjusted to the required temperature.
{{% /alert %}}

The interpretation of the custom recombination functions is based on the `ROOT::TFormula` class \[[@rootformula]\] and supports
all corresponding features, mathematical expressions and constants.


[@shockley-read]: https://doi.org/10.1103/PhysRev.87.835
[@hall]: https://doi.org/10.1103/PhysRev.87.387
[@fossum-lee]: https://doi.org/10.1016/0038-1101(82)90203-9
[@schenk]: https://doi.org/10.1016/0038-1101(92)90184-E
[@fossum-lee2]: https://doi.org/10.1016/0038-1101(83)90173-9
[@kerr]: https://doi.org/10.1063/1.1432476
[@dziewior]: https://doi.org/10.1063/1.89694
