---
# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Impact Ionization"
weight: 5
---

\apsq implements charge multiplication via impact ionization models.
These models are only used by propagation modules which perform a step-by-step simulation of the charge carrier motion.

The gain $g$ is calculated for all models as exponential of the model-dependent impact ionization coefficient $\alpha$ and the length of the step $l$ performed in the respective electric field.
If the electric field strength stays below a configurable threshold $E_{\textrm{thr}}$, unity gain is assumed:
\begin{equation}
    \label{eq:multiplication}
    g (E, T) = \left\{
    \begin{array}{ll}
        e^{l \cdot \alpha(E, T)} & E > E_{\textrm{thr}}\\
        1.0 & E < E_{\textrm{thr}}
    \end{array}
    \right.
\end{equation}

The the following impact ionization models are available:

\subsection{Massey Model}
\label{sec:multi:massey}

The Massey model~\cite{massey} describes impact ionization as a function of the electric field $E$.
The ionization coefficients are parametrized as
\begin{equation}
    \label{eq:multi:massey}
    \alpha (E, T) = A e^{-\frac{B(T)}{E}},
\end{equation}
where $A$ and $B(T)$ are phenomenological parameters, defined for electrons and holes respectively.
While $A$ is assumed to be temperature-independent, parameter $B$ exhibits a temperature dependence and is defined as
\begin{equation}
    B(T) = C + D \cdot T.
\end{equation}

The parameter values implemented in \apsq are taken from Section~3 of~\cite{massey} as:
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
for electrons and holes, respectively.

This model can be selected in the configuration file via the parameter \parameter{multiplication_model = "massey"}.


\subsection{Van Overstraeten-De Man Model}
\label{sec:multi:man}

The Van Overstraeten-De Man model~\cite{overstraeten} describes impact ionization using Chynoweth's law, given by
\begin{equation}
    \label{eq:multi:man}
    \alpha (E, T) = \gamma (T) \cdot a_{\infty} \cdot e^{-\frac{\gamma(T) \cdot b}{E}},
\end{equation}

For holes, two sets of impact ionization parameters $p = \left\{ a_{\infty}, b \right\}$ are used depending on the electric field:
\begin{equation}
    \label{eq:multi:man:h}
    p = \left\{
    \begin{array}{ll}
        p_{\textrm{low}} & E < E_{0}\\
        p_{\textrm{high}} & E > E_{0}
    \end{array}
    \right.
\end{equation}

Temperature scaling of the ionization coefficient is performed  via the $\gamma(T)$ parameter following the Synposys Sentaurus TCAD user manual as:
\begin{equation}
    \label{eq:multi:man:gamma}
    \gamma (T) = \tanh \left(\frac{\SI{0.063e6}{eV}}{2 \SI{8.6173e-5}{eV/K} \cdot T_0} \right) \cdot \tanh \left(\frac{\SI{0.063e6}{eV}}{2 \SI{8.6173e-5}{eV/K} \cdot T} \right)^{-1}
\end{equation}
The value of the reference temperature $T_0$ is not entirely clear as it is never stated explicitly, a value of $T_0 = \SI{300}{K}$ is assumed.
The other parameter values implemented in \apsq are taken from the abstract of~\cite{overstraeten} as:

\begin{equation*}
    \begin{split}
        E_0 &= \SI{4.0e5}{V/cm}\\
        a_{\infty, e} &= \SI{7.03e5}{/cm}\\
        b_{e} &= \SI{1.231e6}{V/cm}\\
    \end{split}
    \qquad
    \begin{split}
        a_{\infty, h, \textrm{low}} &= \SI{1.582e6}{/cm}\\
        a_{\infty, h, \textrm{high}} &= \SI{6.71e5}{/cm}\\
        b_{h, \textrm{low}} &= \SI{2.036e6}{V/cm}\\
        b_{h, \textrm{high}} &= \SI{1.693e6}{V/cm}\\
    \end{split}
\end{equation*}

This model can be selected in the configuration file via the parameter \parameter{multiplication_model = "overstraeten"}.

\subsection{Okuto-Crowell Model}
\label{sec:multi:oku}

The Okuto-Crowell model~\cite{okuto}

This model can be selected in the configuration file via the parameter \parameter{multiplication_model = "okuto"}.

\subsection{Bologna Model}
\label{sec:multi:bologna}

The Bologna model~\cite{bologna}

This model can be selected in the configuration file via the parameter \parameter{multiplication_model = "bologna"}.
