---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Impact Ionization"
weight: 5
---

Allpix Squared implements charge multiplication via impact ionization models.
These models are only used by propagation modules which perform a step-by-step simulation of the charge carrier motion.

The gain $g$ is calculated for all models as exponential of the model-dependent impact ionization coefficient $`\alpha`$ and the length of the step $l$ performed in the respective electric field.
If the electric field strength stays below a configurable threshold $`E_{\textrm{thr}}`$, unity gain is assumed:

```math
\begin{align}
    \label{eq:multiplication}
    g (E, T) &= e^{l \cdot \alpha(E, T)} &\quad E > E_{\textrm{thr}} \nonumber \\
              &= 1.0 &\quad E < E_{\textrm{thr}}
\end{align}
```

The the following impact ionization models are available:

## Massey Model

The Massey model~\cite{massey} describes impact ionization as a function of the electric field $E$.
The ionization coefficients are parametrized as

```math
\begin{equation}
    \label{eq:multi:massey}
    \alpha (E, T) = A e^{-\frac{B(T)}{E}},
\end{equation}
```

where $A$ and $B(T)$ are phenomenological parameters, defined for electrons and holes respectively.
While $A$ is assumed to be temperature-independent, parameter $B$ exhibits a temperature dependence and is defined as

```math
\begin{equation}
    B(T) = C + D \cdot T.
\end{equation}
```

The parameter values implemented in \apsq are taken from Section~3 of~\cite{massey} as:

```math
\begin{equation*}
    \begin{split}
        A_{e} &= \SI{4.43e5}{/cm}\\
        C_{e} &= \SI{9.66e5}{V/cm}\\
        D_{e} &= \SI{4.99e2}{V/cm/K}
    \end{split}
    \qquad
    \begin{split}
        A_{h} &= \SI{1.13e6}{/cm}\\
        C_{h} &= \SI{1.71e6}{V/cm}\\
        D_{h} &= \SI{1.09e3}{V/cm/K}
    \end{split}
\end{equation*}
```

for electrons and holes, respectively.


## Van Overstraeten-De Man Model

The Van Overstraeten-De Man model~\cite{overstraeten}


## Okuto-Crowell Model

The Okuto-Crowell model~\cite{okuto}

## Bologna Model

The Bologna model~\cite{bologna}
