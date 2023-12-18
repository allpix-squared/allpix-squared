---
# SPDX-FileCopyrightText: 2022-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Framework Tools"
weight: 1
---

The following tools are part of the Allpix Squared framework and are located in the `src/tools` directory. They are intended
as centralized components which can be shared between different modules rather than independent tools.

## ROOT and Geant4 utilities

The framework provides a set of methods to ease the integration of ROOT and Geant4 in the framework. An important part is the
extension of the custom conversion `to_string` and `from_string` methods from the internal string utilities (see
[Section 4.8](../04_framework/08_logging.md#internal-utilities)) to support internal ROOT and Geant4 classes. This allows to
directly read configuration parameters to these types, making the code in the modules both shorter and cleaner. In addition,
more conversions functions are provided together with other useful utilities such as the possibility to display a ROOT vector
with units and a thin wrapper for thread-safe ROOT histograms.

## Geant4 Interface

The framework provides an interfacing library with Geant4 that provides alternative run managers to be used by modules
interested in using Geant4 as follows:

1.  `MTRunManager`:
    A run manager for multithreaded event processing. Internally, it creates thread-local managers to handle operations for
    each calling thread independently. It also maintains a stable seed distribution mechanism to ensure results are the same
    regardless of the number of threads that use the manager in parallel.

2.  `RunManager`:
    A run manager for sequential event processing. It uses the same seeding mechanism as the multithreaded version so they
    can be used interchangeably depending on whether multithreading is enabled or not, while ensuring identical results.

The DepositionGeant4 module uses `MTRunManager` to be able to call the `BeamOn` method in parallel on multiple threads thus
benefiting from the multithreading feature while the VisualizationGeant4 module uses `RunManager` to be able to visualize the
particles passage through the detectors.

{{% alert title="Note" color="info" %}}
The `MTRunManager` significantly reduces Geant4's run initialization time (this happens before every event in Allpix Squared)
compared to Geant4's stock run managers (see [Bugzilla/Geant4 2527](https://bugzilla-geant4.kek.jp/show_bug.cgi?id=2527) for
details).

It is not feasible to implement this improvement in the single-threaded `RunManager` since it directly inherits from Geant4's
stock `G4RunManager`. With this run manager, the run initialization time scales with the complexity of the geometry and can -
*in the worst case scenario* - take significantly more time than the actual simulation itself.

Thus it is recommended to use multithreading when using Geant4 in Allpix Squared if allowed by the module configuration.
Allpix Squared allows to use multithreading with only one worker as alternative to `multithreading=false`, though it is
suggested to benchmark the performance for both cases to find the optimal setting for the given geometry.
{{% /alert %}}

## Runge-Kutta integrator

A fast Eigen-powered \[[@eigen3]\] Runge-Kutta integrator is provided as a tool to numerically solve differential equations
\[[@fehlberg]\]. The Runge-Kutta integrator is designed in a generic way and supports multiple methods using different
tableaus. It allows to integrate a system of equations in several steps with customizable step size. The step size can also
be updated during the integration depending on the error of the Runge-Kutta method (if a tableau with error estimation is
used).

The [`GenericPropagation module`](../08_modules/genericpropagation.md) uses Runge-Kutta integrator with the
Runge-Kutta-Fehlberg method (RK5 tableau). After the integrator has been created with the initial position of the charge
carrier to be transported, the `step()` function allows to advance the simulation to the next step.

```cpp
// Define lambda functions to compute the charge carrier velocity at each step
std::function<Eigen::Vector3d(double, Eigen::Vector3d)> carrier_velocity =
    [&](double, Eigen::Vector3d cur_pos) -> Eigen::Vector3d {...};

// Create the Runge-Kutta solver with a RK5 tableau, the carrier velocity function to be used
// as well as the initial timestep and position of the charge carrier
auto runge_kutta = make_runge_kutta(tableau::RK5, carrier_velocity, initial_timestep, position);

// Advance one step with the solver:
auto step = runge_kutta.step();
```

The `getValue()` and `setValue()` methods allow to retrieve, alter and update the position, e.g. to include additional
displacements from diffusion processes.

Furthermore, the `advanceTime()` method can be used to advance the time of the Runge-Kutta solver by the given amount. This
is used for example in situations where the motion is temporarily halted by trapping, and continued when released from the
trap.

### Tableaus

The following Butcher tableaus are implemented and available.
In order to make use of adaptive step size changes, a tableau with error estimation should be chosen.


#### Third-Order Kutta Method (RK3)

This tableau implements a simple and fast third-order Kutta integration which only requires the calculation of three terms.
```math
\begin{array}
{c|ccc}
0                   \\
1/2 & 1/2             \\
1 &  -1 &   2       \\
\hline
  & 1/6 & 2/3 & 1/6 \\
\end{array}
```

#### Fourth-Order Runge-Kutta Method (RK4)

This tableau implements the classical fourth-order Runge-Kutta integration.
```math
\begin{array}
{c|cccc}
  0                 \\
1/2 & 1/2           \\
1/2 &   0 & 1/2     \\
  1 &   0 &   0 & 1 \\
\hline
    & 1/6 & 1/3 & 1/3 & 1/6
\end{array}
```

#### Fourth-Order Runge-Kutta-Fehlberg Method with Error Estimation (RK5)

This tableau implements a fourth-order RKF method with fifth-order error estimation \[[@fehlberg], [@fehlberg2]\].

```math
\begin{array}
{c|cccccc}
    0                                                                     \\
  1/4 &      1/4                                                          \\
  3/8 &      3/32 &       9/32                                            \\
12/13 & 1932/2197 & -7200/2197 &  7296/2197                               \\
    1 &   439/216 &         -8 &   3680/513 &   -845/4104                 \\
  1/2 &     -8/27 &          2 & -3544/2565 &   1859/4104 & -11/40        \\
\hline
      &    16/135 &          0 & 6656/12825 & 28561/56430 &  -9/50 & 2/55 \\
      &    25/216 &          0 &  1408/2565 &   2197/4104 &   -1/5 &    0 \\
\end{array}
```

#### Fourth-Order Runge-Kutta-Cash-Karp Method with Error Estimation (RKCK)

This tableau implements a fourth-order Cash-Karp method with fifth-order error estimation \[[@cashkarp]\].

```math
\begin{array}
{c|cccccc}
   0                                                                            \\
 1/5 &        1/5                                                               \\
3/10 &       3/40 &    9/40                                                     \\
 3/5 &       3/10 &   -9/10 &         6/5                                       \\
   1 &     -11/54 &     5/2 &      -70/27 &        35/27                        \\
 7/8 & 1631/55296 & 175/512 &   575/13824 & 44275/110592 &  253/4096            \\
\hline
     &     37/378 &       0 &     250/621 &      125/594 &         0 & 512/1771 \\
     & 2825/27648 &       0 & 18575/48384 &  13525/55296 & 277/14336 &      1/4 \\

\end{array}
```


## Field Data Parser

A field parser tool is provided, which parses files stored in the INIT or APF file formats and returns field data on a
three-dimensional grid. The number of field components per grid point is configurable via the constructor argument, e.g.
`FieldQuantity::VECTOR` for a vector field or `FieldQuantity::SCALAR` for a scalar field map. The parsed field data is cached
internally by the class, and if a file is requested a second time, the cached field is returned. In conjunction with a static
instance of the field parser class in a module, this allows to share field data across multiple module instances.

```cpp
class MyVectorFieldModule(...) : Module(...) {
private:
    void some_function(std::string canonical_path);
    // Define static field parser instance
    static FieldParser<double> field_parser_;
}

// Create static instance of field parser in the translation unit:
FieldParser<double> MyVectorFieldModule::field_parser_(FieldQuantity::VECTOR);

void MyVectorFieldModule::some_function(std::string canonical_path) {
    // Get vector field from file:
    auto field_data = field_parser_.getByFileName(canonical_path, "V/cm");
}
```

For the INIT format, the `getByFileName()` function of the parser takes the units in which the field data should be
interpreted, and they are automatically converted to the framework base units described in
[Section 3.1](../03_getting_started/01_configuration_files.md#parsing-types-and-units). Fields in the APF format are always
stored in framework base units and do not require conversion. The file path provided to the field parser should always be
canonical, if the file is not found or cannot be parsed, a `std::runtime_error` exception is thrown.

The type of field data to be parsed is automatically deduced from the file content by checking for binary or ASCII text The
field parser determines whether a file is text or binary by checking the first few bytes in the file. If every byte in that
part of the file is non-null, the parser considers the file to be text and reads it as INIT file; otherwise it considers the
file to be binary and parses the field as APF data.


[@eigen3]: http://eigen.tuxfamily.org
[@fehlberg]: https://ntrs.nasa.gov/search.jsp?R=19690021375
[@fehlberg2]: https://doi.org/10.1007%2FBF02234758
[@cashkarp]: https://doi.org/10.1145/79505.79507
