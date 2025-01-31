---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Configuration Files"
weight: 1
---

The framework is configured with simple human-readable configuration files. The configuration format is described in detail
in [Section 4.3](../04_framework/03_configuration.md#file-format). It consists of several section headers within `[` and `]`
brackets, and a section without header at the start. Each of these sections contains a set of key/value pairs separated by
the `=` character. Comments are indicated using the hash symbol (`#`).

The framework has the following three required layers of configuration files:

- The **main** configuration:
  The most important configuration file and the file that is passed directly to the binary. Contains both the global
  framework configuration and the list of modules to instantiate together with their configuration. An example can be found
  in the repository at `examples/example.conf`. More details and a more thorough example are found in
  [Section 3.2](./02_main_configuration.md), several advanced simulation chain configurations are presented in
  [Chapter 9](../09_examples/_index.md).

- The **geometry** configuration:
  It is passed to the framework to determine the detector setup and passive materials. Describes the detector setup,
  containing the position, orientation and model type of all detectors. Optionally, passive materials can be added to this
  configuration. Examples are available in the repository at `examples/ example_detector.conf` or
  `examples/example_detector_passive.conf`. Introduced in [Section 3.3](./03_detector_configuration.md).

- The detector **model** configuration:
  Contains the parameters describing a particular type of detector. Several models are already provided by the framework,
  but new types of detectors can easily be added. See `models/test.conf` in the repository for an example. Please refer to
  [Section 10.5](../10_development/05_new_detector_model.md) for more details about adding new models.

## Parsing Types and Units

The Allpix Squared framework supports the use of a variety of types for all configuration values. The module specifies how
the value type should be interpreted. An error will be raised if either the key is not specified in the configuration file,
the conversion to the desired type is not possible, or if the given value is outside the domain of possible options. Please
refer to the [Chapter 8](../08_modules/_index.md) for the list of module parameters and their types. Parsing the value
roughly follows common-sense (more details can be found in
[Section 4.3](../04_framework/03_configuration.md#accessing-parameters)). A few special rules do apply:

- If the value is a **string**, it may be enclosed by a single pair of double quotation marks (`"`), which are stripped
  before passing the value to the modules. If the string is not enclosed by quotation marks, all whitespace before and
  after the value is erased. If the value is an array of strings, the value is split at every whitespace or comma (`,`)
  that is not enclosed in quotation marks.

- If the value is a **boolean**, either numerical (`0`, `1`) or textual (`false`, `true`) representations are accepted.

- If the value is a **relative path**, that path will be made absolute by adding the absolute path of the directory that
  contains the configuration file where the key is defined.

- If the value is an **arithmetic** type, it may have a suffix indicating the unit. The list of base units is given in the
  table below.

| Quantity                | Default unit                   | Auxiliary units                                                                                                      |
|:------------------------|:-------------------------------|:---------------------------------------------------------------------------------------------------------------------|
| Unity                   | 1                              | -                                                                                                                    |
| Length                  | mm (millimeter)                | nm (nanometer), <br> um (micrometer), <br> cm (centimeter), <br> dm (decimeter), <br> m (meter), <br> km (kilometer) |
| Time                    | ns (nanosecond)                | ps (picosecond), <br> us (microsecond), <br> ms (millisecond), <br> s (second)                                       |
| Energy                  | MeV (megaelectronvolt)         | eV (electronvolt), <br> keV (kiloelectronvolt), <br> GeV (gigaelectronvolt)                                          |
| Temperature             | K (kelvin)                     | -                                                                                                                    |
| Charge                  | e (elementary charge)          | ke (kiloelectrons), <br> fC (femtocoulomb), <br> C (coulomb)                                                         |
| Voltage                 | MV (megavolt)                  | V (volt), <br> kV (kilovolt)                                                                                         |
| Magnetic field strength | kT (kilotesla)                 | T (tesla), <br> mT (millitesla)                                                                                      |
| Angle                   | rad (radian)                   | deg (degree), <br> mrad (milliradian)                                                                                |
| Radiation fluence       | Neq (1-MeV neutron-equivalent) | -                                                                                                                    |

{{% alert title="Warning" color="warning" %}}
If no units are specified, values will always be interpreted in the base units of the framework. In some cases this can lead
to unexpected results. E.g. specifying a bias voltage as `bias_voltage = 50` results in an applied voltage of 50 MV.
Therefore it is strongly recommended to always specify units in the configuration files.
{{% /alert %}}

The internal base units of the framework are not chosen for user convenience but for maximum precision of the calculations
and in order to avoid the necessity of conversions in the code.

Combinations of base units can be specified by using the multiplication sign `*` and the division sign `/` that are parsed in
linear order (thus $`\frac{V m}{s^2}`$ should be specified as `V*m/s/s`). The framework assumes the default units if the unit
is not explicitly specified. It is recommended to always specify the unit explicitly for all parameters that are not
dimensionless as well as for angles.

Examples of specifying key/values pairs of various types are given below:

```ini
# All whitespace at the front and back is removed
first_string =   string_without_quotation
# All whitespace within the quotation marks is preserved
second_string = "  string with quotation marks  "
# Keys are split on whitespace and commas
string_array = "first element" "second element","third element"
# Elements of matrices with more than one dimension are separated
# using square brackets
string_matrix_3x3 = [["1","0","0"], ["0","cos","-sin"], ["0","sin",cos]]
# If the matrix is of dimension 1xN, the outer brackets have to be
# added explicitly
integer_matrix_1x3 = [[10, 11, 12]]
# Integer and floats can be specified in standard formats
int_value = 42
float_value = 123.456e9
# Units can be passed to arithmetic types
energy_value = 1.23MeV
time_value = 42ns
# Units are combined in linear order without grouping or implicit brackets
acceleration_value = 1.0m/s/s
# Thus the quantity below is the same as 1.0deg*kV*K/m/s
random_quantity = 1.0deg*kV/m/s*K
# Relative paths are expanded to absolute paths, e.g. the following value
# will become "/home/user/test" if the configuration file is located
# at "/home/user"
output_path = "test"
# Booleans can be represented in numerical or textual style
my_switch = true
my_other_switch = 0
```

{{% alert title="Note" color="info" %}}
In some places, providing configuration variables with units is mandatory. In case the respective input should be interpreted as base units, or without units such as a weighting potential, the parameter can be provided as empty string, i.e. `observable_units = ""`.
{{% /alert %}}
