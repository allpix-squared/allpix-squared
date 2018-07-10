# WeightingFieldReader
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Development

### Description
Adds a weighting field (Ramo field) to the detector from one of the supported sources. By default, detectors do not have a weighting field applied.
This module support two types of weighting fields.
The field can either read in from a file in the INIT format using the parameter `model = "init"`, e.g. from an electrostatic TCAD simulation.
A converter tool for fields from adaptive TCAD meshes is provided with the framework.

Alternatively, the model can be set to **pad** and the weighting field of a pixel in a plane condenser is calculated numerically from first principles, following the procedure described in [@planecondenser].
The z-component of the weighting field is given by

$`E_w^z/V_w = \frac{1}{2\pi}g_z(x. y, z) + \frac{1}{2\pi}\sum_{n = 1}^N\left[g_z(x, y, 2nd + z) + g_z(x, y, 2nd - z)\right] `$

with

$` g_z(x, y, u) = \frac{x_1 y_1 (x_1^2 + y_1^2 + 2u^2)}{(x_1^2 + u^2)(y_1^2 + u^2)\sqrt{x_1^2 + y_1^2 + u^2}} + \frac{x_2 y_2 (x_2^2 + y_2^2 + 2u^2)}{(x_2^2 + u^2)(y_2^2 + u^2)\sqrt{x_2^2 + y_2^2 + u^2}} + \frac{x_1 y_2 (x_1^2 + y_2^2 + 2u^2)}{(x_1^2 + u^2)(y_2^2 + u^2)\sqrt{x_1^2 + y_2^2 + u^2}} + \frac{x_2 y_1 (x_2^2 + y_1^2 + 2u^2)}{(x_2^2 + u^2)(y_1^2 + u^2)\sqrt{x_2^2 + y_1^2 + u^2}} `$

where $`x_{1,2} = x \pm \frac{w_x}{2} \qquad y_{1,2} = y \pm \frac{w_y}{2}`$.

Here, $`w_{x,y}`$ indicate the size of the collection electrode (i.e. the implant), $`V_w`$ is the potential of the electrode and *d* is the thickness of the sensor.

The *x* and *y* components of the field are determined in the same way, i.e. by calculating the derivatives from the weighting potential as

$` E_w^{x,y} = - \frac{\partial \phi_w}{\partial (x, y)}`$.

The integral part of the expressions for the weighting field and potential are ignored.
In [@planecondenser] it has been shown that the uncertainty on the weighting field is smaller than

$`| \Delta E_w^z | < \frac{V_w}{8\pi} \frac{w_xw_y}{d^3} \frac{1}{N^2}`$,

where *N* the expansion of the series.
In this implementation, a value of $`N = 100`$ is used.

### Parameters
* `model` : Type of the weighting field model, either **init** or **pad**.
* `file_name` : Location of file containing the weighting field in the INIT format. Only used if the *model* parameter has the value **init**.

### Usage
An example to add a weighting field via an INIT file to the detector called "dut" is given below.

```ini
[WeightingFieldReader]
name = "dut"
model = "init"
file_name = "example_weighting_field.init"
```

[@planecondenser]: https://doi.org/10.1016/j.nima.2014.08.044
