---
# SPDX-FileCopyrightText: 2021-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Impact Ionization"
weight: 5
---

Allpix Squared implements charge multiplication via impact ionization models. These models are only used by propagation
modules which perform a step-by-step simulation of the charge carrier motion.

The per-step gain $`g`$ is calculated for all models as exponential of the model-dependent impact ionization coefficient $`\alpha`$ and
the length of the step $`l`$ performed in the respective electric field. If the electric field strength stays below a
configurable threshold $`E_{\text{thr}}`$, unity gain is assumed:

```math
g (E, T) = \left\{
\begin{array}{ll}
    e^{l \cdot \alpha(E, T)} & E > E_{\text{thr}}\\
    1.0 & E < E_{\text{thr}}
\end{array}
\right.
```

The impact ionization coefficient $`\alpha`$ is calculated depending on the selected impact ionization model. The models themselves are described below.

The number of additional charge carriers generated per step $`n`$ is determined via a stochastic approach by applying the following equation dependent on a random number drawn from a uniform distribution $`u(0,1)`$
```math
n = \frac{\ln(u)}{\ln(1-1/g)} = \frac{1}{\log_u(1-1/g)}
```
This distribution is applied e.g. in Garfield++\[[@garfieldpp]\] and represents a microscopic simulation of Yule processes.

The number of secondary charge carriers generated from impact ionization is calculated for every individual charge carrier within a group of charge carriers and summed per propagation step. Additional charge carriers are then added to the group (same-type carriers) or deposited (opposite-type) at the end of the corresponding step.

This algorithm results in a mean number of secondaries generated equal to
```math
\langle n_{total}\rangle = \exp\left(\int_{x_0}^{x_n}\alpha(x)dx \right)
```
for sufficiently low step sizes.

The following impact ionization models are available:

## Massey Model

The Massey model \[[@massey]\] describes impact ionization as a function of the electric field $`E`$.
The ionization coefficients are parametrized as

```math
\alpha (E, T) = A e^{-\frac{B(T)}{E}},
```

where $`A`$ and $`B(T)`$ are phenomenological parameters, defined for electrons and holes respectively.
While $`A`$ is assumed to be temperature-independent, parameter $`B`$ exhibits a temperature dependence and is defined as

```math
B(T) = C + D \cdot T.
```

### Original publication

The parameter values implemented in Allpix Squared are taken from Section 3 of \[[@massey]\] as:

```math
\begin{aligned}
    A_{e} &= 4.43\times 10^{5} \,\text{/cm}\\
    C_{e} &= 9.66\times 10^{5} \,\text{V/cm}\\
    D_{e} &= 4.99\times 10^{2} \,\text{V/cm/K}\\
\\
    A_{h} &= 1.13\times 10^{6} \,\text{/cm}\\
    C_{h} &= 1.71\times 10^{6} \,\text{V/cm}\\
    D_{h} &= 1.09\times 10^{3} \,\text{V/cm/K}\\
\end{aligned}
```

for electrons and holes, respectively.

This model can be selected in the configuration file via the parameter `multiplication_model = "massey"`.

### Optimized parameters

An optimized parametrization of the Massey model based on measurements with an infrared laser is implemented in Allpix Squared, based on Table 2 of \[[@rd50ionization]\] with the values:

```math
\begin{aligned}
    A_{e} &= 1.186\times 10^{6} \,\text{/cm}\\
    C_{e} &= 1.020\times 10^{6} \,\text{V/cm}\\
    D_{e} &= 1.043\times 10^{3} \,\text{V/cm/K}\\
\\
    A_{h} &= 2.250\times 10^{6} \,\text{/cm}\\
    C_{h} &= 1.851\times 10^{6} \,\text{V/cm}\\
    D_{h} &= 1.828\times 10^{3} \,\text{V/cm/K}\\
\end{aligned}
```

for electrons and holes, respectively.

This model can be selected in the configuration file via the parameter `multiplication_model = "massey_optimized"`.


## Van Overstraeten-De Man Model

The Van Overstraeten-De Man model \[[@overstraeten]\] describes impact ionization using Chynoweth's law, given by

```math
\alpha (E, T) = \gamma (T) \cdot a_{\infty} \cdot e^{-\frac{\gamma(T) \cdot b}{E}},
```

For holes, two sets of impact ionization parameters $`p = \left\{ a_{\infty}, b \right\}`$ are used depending on the electric field:

```math
p = \left\{
\begin{array}{ll}
    p_{\text{low}} & E < E_{0}\\
    p_{\text{high}} & E > E_{0}
\end{array}
\right.
```

Temperature scaling of the ionization coefficient is performed via the $`\gamma(T)`$ parameter following the Synposys
Sentaurus TCAD user manual as:

```math
\gamma (T) = \tanh \left(\frac{\hbar \omega_{op}}{2 k_{\mathrm{B}} \cdot T_0} \right) \cdot \tanh \left(\frac{\hbar \omega_{op}}{2 k_{\mathrm{B}} \cdot T} \right)^{-1}
```

with $`\hbar \omega_{op} = 0.063 \,\text{eV}`$ and the Boltzmann constant $`k_{\mathrm{B}} = 8.6173\times 10^{-5} \,\text{eV/K}`$. The value of the reference temperature $`T_0`$ is not entirely clear as it is never
stated explicitly, a value of $`T_0 = 300 \,\text{K}`$ is assumed.

### Original publication

The other model parameter values implemented in Allpix Squared are taken from the abstract
of \[[@overstraeten]\] as:

```math
\begin{aligned}
    E_0 &= 4.0\times 10^{5} \,\text{V/cm}\\
    a_{\infty, e} &= 7.03\times 10^{5} \,\text{/cm}\\
    b_{e} &= 1.231\times 10^{6} \,\text{V/cm}\\
    \\
    a_{\infty, h, \text{low}} &= 1.582\times 10^{6} \,\text{/cm}\\
    a_{\infty, h, \text{high}} &= 6.71\times 10^{5} \,\text{/cm}\\
    b_{h, \text{low}} &= 2.036\times 10^{6} \,\text{V/cm}\\
    b_{h, \text{high}} &= 1.693\times 10^{6} \,\text{V/cm}
\end{aligned}
```

This model can be selected in the configuration file via the parameter `multiplication_model = "overstraeten"`.


### Optimized parameters

An optimized parametrization of the Van Overstraeten-De Man model based on measurements with an infrared laser is implemented in Allpix Squared, based on Table 3 of \[[@rd50ionization]\] with the following parameter values:

```math
\begin{aligned}
    a_{\infty, e} &= 1.149\times 10^{6} \,\text{/cm}\\
    b_{e} &= 1.325\times 10^{6} \,\text{V/cm}\\
    \\
    a_{\infty, h} &= 2.519\times 10^{6} \,\text{/cm}\\
    b_{h} &= 2.428\times 10^{6} \,\text{V/cm}\\
    \\
    \hbar \omega_{op} &= 0.0758 \,\text{eV}
\end{aligned}
```

In contrast to the original model, this publication uses a parametrization without differentiating between low and high field regions, hence only one parameter value is provided for each of $`a_{\infty, h}`$ and $`b_{h}`$.

This model can be selected in the configuration file via the parameter `multiplication_model = "overstraeten_optimized"`.



## Okuto-Crowell Model

The Okuto-Crowell model \[[@okuto]\] defines the impact ionization coefficient similarly to the above models but in addition
features a linear dependence on the electric field strength $`E`$. The coefficient is given by:

```math
\alpha (E, T) = a(T) \cdot E \cdot e^{-\left(\frac{b(T)}{E}\right)^2}.
```

The two parameters $`a, b`$ are temperature dependent and scale with respect to the reference temperature
$`T_0 = 300 \,\text{K}`$ as:

```math
\begin{aligned}
    a(T) &= a_{300} \left[ 1 + c\left(T - T_0\right) \right]\\
    b(T) &= a_{300} \left[ 1 + d\left(T - T_0\right) \right]
\end{aligned}
```

### Original publication

The parameter values implemented in Allpix Squared are taken from Table 1 of \[[@okuto]\], using the values for silicon, as:

```math
\begin{aligned}
    a_{300, e} &= 0.426 \,\text{/V}\\
    c_{e} &= 3.05\times 10^{-4}\\
    b_{300, e} &= 4.81\times 10^{5} \,\text{V/cm}\\
    d_{e} &= 6.86\times 10^{-4}\\
\\
    a_{300, h} &= 0.243 \,\text{/V}\\
    c_{h} &= 5.35\times 10^{-4}\\
    b_{300, h} &= 6.53\times 10^{5} \,\text{V/cm}\\
    d_{h} &= 5.67\times 10^{-4}\\
\end{aligned}
```

This model can be selected in the configuration file via the parameter `multiplication_model = "okuto"`.

### Optimized parameters

An optimized parametrization of the Okuto-Crowell model based on measurements with an infrared laser is implemented in Allpix Squared, based on Table 4 of \[[@rd50ionization]\] with the following parameter values:

```math
\begin{aligned}
    a_{300, e} &= 0.289 \,\text{/V}\\
    c_{e} &= 9.03\times 10^{-4}\\
    b_{300, e} &= 4.01\times 10^{5} \,\text{V/cm}\\
    d_{e} &= 1.11\times 10^{-3}\\
\\
    a_{300, h} &= 0.202 \,\text{/V}\\
    c_{h} &= -2.20\times 10^{-3}\\
    b_{300, h} &= 6.40\times 10^{5} \,\text{V/cm}\\
    d_{h} &= 8.25\times 10^{-4}\\
\end{aligned}
```

This model can be selected in the configuration file via the parameter `multiplication_model = "okuto_optimized"`.



## Bologna Model

The Bologna model \[[@bologna]\] describes impact ionization for experimental data in an electric field range from
$`130 \,\text{kV/cm}`$ to $`230 \,\text{kV/cm}`$ and temperatures up to $`400 \,\text{°C}`$. The impact ionization
coefficient takes a different form than the previous models and is given by

```math
\alpha (E, T) = \frac{E}{a(T) + b(T) e^{d(T) / \left(E + c(T) \right)}},
```

for both electrons and holes.
The temperature-dependent parameters $`a(T), b(T), c(T)`$ and $`d(T)`$ are defined as:

```math
\begin{aligned}
    a(T) &= a_{0} + a_1 T^{a_2}\\
    b(T) &= b_{0} e^{b_1 T}\\
    c(T) &= c_{0} + c_1 T^{c_2} + c_3 T^{2}\\
    d(T) &= d_{0} + d_1 T + d_2 T^{2}\\
\end{aligned}
```

The parameter values implemented in Allpix Squared are taken from Table 1 of \[[@bologna]\] as:

```math
\begin{aligned}
    a_{0, e} &= 4.3383 \,\text{V}\\
    a_{1, e} &= -2.42\times 10^{-12} \,\text{V}\\
    a_{2, e} &= 4.1233\\
    b_{0, e} &= 0.235 \,\text{V}\\
    b_{1, e} &= 0\\
    c_{0, e} &= 1.6831\times 10^{4} \,\text{V/cm}\\
    c_{1, e} &= 4.3796 \,\text{V/cm}\\
    c_{2, e} &= 1\\
    c_{3, e} &= 0.13005 \,\text{V/cm}\\
    d_{0, e} &= 1.2337\times 10^{6} \,\text{V/cm}\\
    d_{1, e} &= 1.2039\times 10^{3} \,\text{V/cm}\\
    d_{2, e} &= 0.56703 \,\text{V/cm}\\
\\
    a_{0, h} &= 2.376 \,\text{V}\\
    a_{1, h} &= 1.033\times 10^{-2} \,\text{V}\\
    a_{2, h} &= 1\\
    b_{0, h} &= 0.17714 \,\text{V}\\
    b_{1, h} &= -2.178\times 10^{-3} \,\text{/K}\\
    c_{1, h} &= 0\\
    c_{1, h} &= 9.47\times 10^{-3} \,\text{V/cm}\\
    c_{2, h} &= 2.4924\\
    c_{3, h} &= 0\\
    d_{0, h} &= 1.4043\times 10^{6} \,\text{V/cm}\\
    d_{1, h} &= 2.9744\times 10^{3} \,\text{V/cm}\\
    d_{2, h} &= 1.4829 \,\text{V/cm}\\
\end{aligned}
```

This model can be selected in the configuration file via the parameter `multiplication_model = "bologna"`.

## Custom Impact Ionization Models

Allpix Squared provides the possibility to use fully custom impact ionization models. In order to use a custom model, the
parameter `multiplication_model = "custom"` needs to be set in the configuration file. Additionally, the following
configuration keys have to be provided:

- `multiplication_function_electrons`:
  The formula describing the electron impact ionization gain.

- `multiplication_function_holes`:
  The formula describing the hole impact ionization gain.

The functions defined via these parameters can depend on the local electric field. In order to use the electric field
magnitude in the formula, an `x` has to be placed at the respective position.

Parameters of the functions can either be placed directly in the formulas in framework-internal units, or provided separately
as arrays via the `multiplication_parameters_electrons` and `multiplication_parameters_electrons`. Placeholders for
parameters in the formula are denoted with squared brackets and a parameter number, for example `[0]` for the first
parameter provided. Parameters specified separately from the formula can contain units which will be interpreted
automatically.

{{% alert title="Warning" color="warning" %}}
Parameters directly placed in the impact ionization formula have to be supplied in framework-internal units since the
function will be evaluated with both electric field strength and doping concentration in internal units. It is recommended
to use the possibility of separately configuring the parameters and to make use of units to avoid conversion mistakes.
{{% /alert %}}

{{% alert title="Warning" color="warning" %}}
It should be noted that the temperature passed via the module configuration is not evaluated for the custom impact ionization
model, but the model parameters need to be manually adjusted to the required temperature.
{{% /alert %}}

The interpretation of the custom impact ionization functions is based on the `ROOT::TFormula` class \[[@rootformula]\] and
supports all corresponding features, mathematical expressions and constants.


[@garfieldpp]: https://gitlab.cern.ch/garfield/garfieldpp
[@massey]: https://doi.org/10.1109/TED.2006.881010
[@rd50ionization]: https://arxiv.org/abs/2211.16543
[@overstraeten]: https://doi.org/10.1016/0038-1101(70)90139-5
[@okuto]: https://doi.org/10.1016/0038-1101(75)90099-4
[@bologna]: https://doi.org/10.1109/SISPAD.1999.799251
[@rootformula]: https://root.cern.ch/doc/master/classTFormula.html
