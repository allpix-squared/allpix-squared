---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Configuration"
weight: 2
---

#### How do I run a module only for one detector?

This is only possible for detector modules (which are constructed to work on individual detectors). To run it on a single
detector, one should add a parameter `name` specifying the name of the detector (as defined in the detector configuration
file):

```ini
[ElectricFieldReader]
name = "dut"
model = "mesh"
file_name = "../example_electric_field.init"
```

#### How do I run a module only for a specific detector type?

This is only possible for detector modules (which are constructed to work on individual detectors). To run it for a specific
type of detector, one should add a parameter `type` with the type of the detector model (as set in the detector configuration
file by the `model` parameter):

```ini
[ElectricFieldReader]
type = "timepix"
model = "linear"
bias_voltage = -50V
depletion_voltage = -30V
```

Please refer to [Section 4.3](../04_framework/04_modules.md#module-instantiation) for more information.

#### How can I run the same type of module with different settings?

This is possible by using the `input` and `output` parameters of a module that specify the messages of the module:

```ini
[DefaultDigitizer]
name = "dut0"
adc_resolution = 4
output = "low_adc_resolution"

[DefaultDigitizer]
name = "dut0"
adc_resolution = 12
output = "high_adc_resolution"
```

By default, both the input and the output of module are messages with an empty name. In order to further process the data,
subsequent modules require the `input` parameter to not receive multiple messages:

```ini
[DetectorHistogrammer]
input = "low_adc_resolution"
name = "dut0"

[DetectorHistogrammer]
input = "high_adc_resolution"
name = "dut0"
```

Please refer to [Section 4.6](../04_framework/06_messages.md) for more information.

#### How can I temporarily ignore a module during development?

The section header of a particular module in the configuration file can be replaced by the string `Ignore`. The section and
all of its key/value pairs are then ignored. Modules can also be excluded from the compilation process as explained in
[Section 2.5](../02_installation/05_cmake_configuration.md).

#### Can I get a high verbosity level only for a specific module?

Yes, it is possible to specify verbosity levels and log formats per module. This can be done by adding the `log_level` and/or
`log_format` key to a specific module to replace the parameter in the global configuration sections.

#### Can I import an electric field from TCAD and use it for simulating propagation?

Yes, the framework includes a tool to convert DF-ISE files from TCAD to an internal format which Allpix Squared can parse.
More information about this tool can be found in [Section 14.2](../14_additional/mesh_converter.md), instructions to import
the generated field are provided in [Section 3.7](../03_getting_started/07_extending_simulation_chain.md#electric-fields).

#### What parameters should I consider when writig a simulation for a non-silicon sensor?

While Allpix Squared implements several material-dependent default parameters, other parameters and models default to values
suitable for silicon sensors. It is in any case advisable to check the following configuration parameters to ensure
consistent results.

- Sensor material:
  The parameter `sensor_material`, to be adjusted in the corresponding detector model file, is crucial for the particle
  interaction simulated via Geant4 and defines further default parameters.

- Charge creation energy:
  The parameter `charge_creation_energy` is available in several modules for energy deposition and provides a material
  dependent default. For default values refer to [Section 6.1](../06_models/01_material_properties.md).

- Fano factor:
  The parameter `fano_factor` is available in several modules for energy deposition and provides a material dependent
  default. For default values refer to [Section 6.1](../06_models/01_material_properties.md).

- Mobility Model:
  The parameter `mobility_model` needs to be adapted to the sensor material by the user.
  [Section 6.2](../06_models/02_carrier_mobility.md) lists the available models.

- Recombination Model:
  The parameter `recombination_model` can be adapted by the user. [Section 6.3](../06_models/03_lifetime_recombination.md)
  lists the available models.
