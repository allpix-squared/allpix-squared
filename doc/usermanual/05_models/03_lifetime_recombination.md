---
title: "Charge Carrier Lifetime & Recombination"
weight: 3
---

Allpix Squared provides the possibility to simulate finite lifetimes of
charge carriers as a function of the local doping concentration via
non-radiative recombination processes. While most of these models
require the *total doping concentration* $`N_D + N_A`$ as parameter,
the doping profile used throughout Allpix Squared provides the
*effective doping concentration* $`N_D - N_A`$ since this also
encodes the majority charge carriers via its sign - an information
relevant to some of the models. However, in the parts of a silicon
detector relevant for this simulation, i.e. the sensing volume, the
difference between effective and total concentration is expected to be
negligible. Therefore the two values are treated as equivalent
throughout the lifetime models and the doping concentration is taken as
the absolute value $`N = \left|N_D - N_A\right|`$.

Whether a charge carrier has recombined with the lattice is calculated
for every step of the simulation using the relation

```math
p < 1 - e^{- dt / \tau(N)}
```

where $`p`$ is a recombination probability, drawn from a uniform
distribution with $`[0, 1]`$, $`dt`$ is the last time step of the charge
carrier motion and $`\tau`$ the lifetime for the local doping concentration
calculated by the models described in the following. If the above equation
evaluates to *false*, the charge carrier still exists, if it evaluates to
*true* it has been recombined with the lattice.

Finite charge carrier lifetime can be simulated by all propagation
modules and comprise the following models:

## Shockley-Read-Hall Recombination

This model describes the finite lifetime based on Shockley-Read-Hall or
trap-assisted recombination of charge carriers with the lattice
\[[@shockley-read], [@hall]\]. The lifetime is calculated using the
Shockley-Read-Hall relation as given by \[[@fossum-lee]\]:

```math
\tau(N) = \frac{\tau_0}{1 + \frac{N}{N_{d0}}}
```

where $`\tau_0`$ and $`N_{d0}`$ are reference lifetime and
doping concentration, for electrons and holes respectively. The
parameter values implemented in Allpix Squared are taken from
\[[@fossum-lee]\] and the Synopsys Sentaurus TCAD software manual as:

```math
\begin{align*}
\tau_{0,e} &= 1 \times 10^{-5}\ \text{s} \newline
N_{d0,e}   &= 1 \times 10^{16}\ \text{cm}^{-3}
\end{align*}
```

for electrons and

```math
\begin{align*}
\tau_{0,h} &= 4.0 \times 10^{-4}\ \text{s} \newline
N_{d0,h}   &= 7.1 \times 10^{15}\ \text{cm}^{-3}
\end{align*}
```

for holes.

The temperature dependence of the Shockley-Read-Hall lifetime is scaled
following the low-temperature approximation model presented \[[@schenk]\]
as:

```math
\tau(N, T) = \tau(N) \cdot \left( \frac{300\ \text{K}}{T} \right)^{3/2}
```

This model can be selected in the configuration file via the parameter
`recombination_model = "srh"`.

## Auger Recombination

At high doping levels exceeding $`5 \times 10^{18}\ \text{cm}^{-3}`$
\[[@fossum-lee]\], the Auger recombination model becomes increasingly
important. It assumes that the excess energy created by electron-hole
recombinations is transferred to another electron (*e-e-h process*) or
another hole (*e-h-h process*). The total recombination rate is then given
by \[[@kerr]\]:

```math
R_{Auger} = C_n n^2p + C_p n p^2
```

where $`C_n`$ and $`C_p`$ are the Auger coefficients. The first term
corresponds to the e-e-h process and the second term to the e-h-h process.
In highly-doped silicon, the Auger lifetime for minority charge carriers
can be written as:

```math
\tau(N) = \frac{1}{C_{a} \cdot N^2}
```

where $`C_{a} = C_{n} + C_{p}`$ is the ambipolar Auger coefficient, taken as
$`C_{a} = 3.8 \times 10^{-31}\ \text{cm}^6\ \text{s}^{-1}`$ from
\[[@dziewior]\].

This recombination mode applies to minority charge carriers only,
majority charge carriers have an infinite life time under this model and
the recombination equation will always evaluate to *true*.

This model can be selected in the configuration file via the parameter
`recombination_model = "auger"`.

## Combined SRH/Auger Recombination

This model combines the charge carrier recombination from the
Shockley-Read-Hall and the Auger model by inversely summing the
individual lifetimes calculated by the models via

```math
\begin{align*}
\tau^{-1}(N) &= \tau_{srh}^{-1}(N) + \tau_{a}^{-1}(N) &\quad \text{for minority charge carriers} \newline
             &= \tau_{srh}^{-1}(N)                    &\quad \text{for majority charge carriers}
\end{align*}
```

where $`\tau_{srh}(N)`$ is the Shockley-Read-Hall and $`\tau_{a}(N)`$ the
Auger lifetime. The latter is only taken into account for minority charge
carriers.

This model can be selected in the configuration file via the parameter
`recombination_model = "srh_auger"`.

## Recombination with Constant Lifetimes

Some materials require constant lifetimes for charge carriers
$`\tau(N) = \tau`$. This model requires the additional configuration
keys `lifetime_electron` and `lifetime_hole` to be present in the
module configuration section, for example:

```ini
# Constant lifetimes for electrons and holes in GaAs with Cr compensation:
recombination_model = "constant"
lifetime_electron = 30ns
lifetime_hole = 4.5ns
```

This model can be selected in the configuration file via the parameter
`recombination_model = "constant"`.


[@shockley-read]: https://doi.org/10.1103/PhysRev.87.835
[@hall]: https://doi.org/10.1103/PhysRev.87.387
[@fossum-lee]: https://doi.org/10.1016/0038-1101(82)90203-9
[@schenk]: https://doi.org/10.1016/0038-1101(92)90184-E
[@fossum-lee]: https://doi.org/10.1016/0038-1101(83)90173-9
[@kerr]: https://doi.org/10.1063/1.1432476
[@dziewior]: https://doi.org/10.1063/1.89694
