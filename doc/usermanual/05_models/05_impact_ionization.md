---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Impact Ionization"
weight: 5
---

Allpix Squared implements charge multiplication via impact ionization models. These models are only used by propagation
modules which perform a step-by-step simulation of the charge carrier motion.

The gain $`g`$ is calculated for all models as exponential of the model-dependent impact ionization coefficient $`\alpha`$ and
the length of the step $`l`$ performed in the respective electric field. If the electric field strength stays below a
configurable threshold $`E_{\text{thr}}`$, unity gain is assumed:

```math
\begin{equation}
    g (E, T) = \left\{
    \begin{array}{ll}
        e^{l \cdot \alpha(E, T)} & E > E_{\text{thr}}\\
        1.0 & E < E_{\text{thr}}
    \end{array}
    \right.
\end{equation}
```

The the following impact ionization models are available:

## Massey Model

The Massey model \[[@massey]\] describes impact ionization as a function of the electric field $`E`$.
The ionization coefficients are parametrized as

```math
\begin{equation}
    \alpha (E, T) = A e^{-\frac{B(T)}{E}},
\end{equation}
```

where $`A`$ and $`B(T)`$ are phenomenological parameters, defined for electrons and holes respectively.
While $`A`$ is assumed to be temperature-independent, parameter $`B`$ exhibits a temperature dependence and is defined as

```math
\begin{equation}
    B(T) = C + D \cdot T.
\end{equation}
```

The parameter values implemented in Allpix Squared are taken from Section 3 of \[[@massey]\] as:

```math
\begin{equation*}
    \begin{split}
        A_{e} &= 4.43\times 10^{5} \,\text{/cm}\\
        C_{e} &= 9.66\times 10^{5} \,\text{V/cm}\\
        D_{e} &= 4.99\times 10^{2} \,\text{V/cm/K}
    \end{split}
    \qquad
    \begin{split}
        A_{h} &= 1.13\times 10^{6} \,\text{/cm}\\
        C_{h} &= 1.71\times 10^{6} \,\text{V/cm}\\
        D_{h} &= 1.09\times 10^{3} \,\text{V/cm/K}
    \end{split}
\end{equation*}
```

for electrons and holes, respectively.

This model can be selected in the configuration file via the parameter `multiplication_model = "massey"`.


## Van Overstraeten-De Man Model

The Van Overstraeten-De Man model \[[@overstraeten]\] describes impact ionization using Chynoweth's law, given by

```math
\begin{equation}
    \alpha (E, T) = \gamma (T) \cdot a_{\infty} \cdot e^{-\frac{\gamma(T) \cdot b}{E}},
\end{equation}
```

For holes, two sets of impact ionization parameters $`p = \left\{ a_{\infty}, b \right\}`$ are used depending on the electric field:

```math
\begin{equation}
    p = \left\{
    \begin{array}{ll}
        p_{\text{low}} & E < E_{0}\\
        p_{\text{high}} & E > E_{0}
    \end{array}
    \right.
\end{equation}
```

Temperature scaling of the ionization coefficient is performed  via the $`\gamma(T)`$ parameter following the Synposys
Sentaurus TCAD user manual as:

```math
\begin{equation}
    \gamma (T) = \tanh \left(\frac{0.063\times 10^{6} \,\text{eV}}{2 \times 8.6173\times 10^{-5} \,\text{eV/K} \cdot T_0} \right) \cdot \tanh \left(\frac{0.063\times 10^{6} \,\text{eV}}{2 \times 8.6173\times 10^{-5} \,\text{eV/K} \cdot T} \right)^{-1}
\end{equation}
```

The value of the reference temperature $`T_0`$ is not entirely clear as it is never stated explicitly, a value of
$`T_0 = 300 \,\text{K}`$ is assumed. The other parameter values implemented in Allpix Squared are taken from the abstract
of \[[@overstraeten]\] as:

```math
\begin{equation*}
    \begin{split}
        E_0 &= 4.0\times 10^{5} \,\text{V/cm}\\
        a_{\infty, e} &= 7.03\times 10^{5} \,\text{/cm}\\
        b_{e} &= 1.231\times 10^{6} \,\text{V/cm}\\
    \end{split}
    \qquad
    \begin{split}
        a_{\infty, h, \text{low}} &= 1.582\times 10^{6} \,\text{/cm}\\
        a_{\infty, h, \text{high}} &= 6.71\times 10^{5} \,\text{/cm}\\
        b_{h, \text{low}} &= 2.036\times 10^{6} \,\text{V/cm}\\
        b_{h, \text{high}} &= 1.693\times 10^{6} \,\text{V/cm}\\
    \end{split}
\end{equation*}
```

This model can be selected in the configuration file via the parameter `multiplication_model = "overstraeten"`.

## Okuto-Crowell Model

The Okuto-Crowell model \[[@okuto]\] defines the impact ionization coefficient similarly to the above models but in addition
features a linear dependence on the electric field strength $`E`$. The coefficient is given by:

```math
\begin{equation}
    \alpha (E, T) = a(T) \cdot E \cdot e^{-\left(\frac{b(T)}{E}\right)^2}.
\end{equation}
```

The two parameters $`a, b`$ are temperature dependent and scale with respect to the reference temperature
$`T_0 = 300 \,\text{K}`$ as:

```math
\begin{equation}
    \begin{split}
        a(T) &= a_{300} \left[ 1 + c\left(T - T_0\right) \right]\\
        b(T) &= a_{300} \left[ 1 + d\left(T - T_0\right) \right]
    \end{split}
\end{equation}
```

The parameter values implemented in Allpix Squared are taken from Table 1 of \[[@okuto]\], using the values for silicon, as:

```math
\begin{equation*}
    \begin{split}
        a_{300, e} &= 0.426 \,\text{/V}\\
        c_{e} &= 3.05\times 10^{-4}\\
        b_{300, e} &= 4.81\times 10^{5} \,\text{V/cm}\\
        d_{e} &= 6.86\times 10^{-4}\\
    \end{split}
    \qquad
    \begin{split}
        a_{300, h} &= 0.243 \,\text{/cm}\\
        c_{h} &= 5.35\times 10^{-4}\\
        b_{300, h} &= 6.53\times 10^{5} \,\text{V/cm}\\
        d_{h} &= 5.67\times 10^{-4}\\
    \end{split}
\end{equation*}
```

This model can be selected in the configuration file via the parameter `multiplication_model = "okuto"`.

## Bologna Model

The Bologna model \[[@bologna]\] describes impact ionization for experimental data in an electric field range from
$`130 \,\text{kV/cm}`$ to $`230 \,\text{kV/cm}`$ and temperatures up to $`400 \,^{\circ}\text{C}`$. The impact ionization
coefficient takes a different form than the previous models and is given by

```math
\begin{equation}
    \alpha (E, T) = \frac{E}{a(T) + b(T) e^{d(T) / \left(E + c(T) \right)}},
\end{equation}
```

for both electrons and holes.
The temperature-dependent parameters $`a(T), b(T), c(T)`$ and $`d(T)`$ are defined as:

```math
\begin{equation}
    \begin{split}
        a(T) &= a_{0} + a_1 T^{a_2}\\
        b(T) &= b_{0} e^{b_1 T}\\
        c(T) &= c_{0} + c_1 T^{c_2} + c_3 T^{2}\\
        d(T) &= d_{0} + d_1 T + d_2 T^{2}
    \end{split}
\end{equation}
```

The parameter values implemented in Allpix Squared are taken from Table 1 of \[[@bologna]\] as:

```math
\begin{equation*}
    \begin{split}
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
    \end{split}
    \qquad
    \begin{split}
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
    \end{split}
\end{equation*}
```

This model can be selected in the configuration file via the parameter `multiplication_model = "bologna"`.
