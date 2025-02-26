---
# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Configuration and Parameters"
weight: 3
---

Individual modules as well as the framework itself are configured through configuration files, which all follow the same
format. Explanations on how to use the various configuration files together with several examples have been provided in
[Section 3.1](../03_getting_started/01_configuration_files.md).

## File format

Throughout the framework, a simplified version of TOML \[[@tomlgit]\] is used as standard format for configuration files. The
format is defined as follows:

1. All whitespace at the beginning or end of a line are stripped by the parser. In the rest of this format specification the
   *line* refers to the line with this whitespace stripped.

2. Empty lines are ignored.

3. Every non-empty line should start with either `#`, `[` or an alphanumeric character. Every other character should lead to
   an immediate parse error.

4. If the line starts with a hash character (`#`), it is interpreted as comment and all other content on the same line is
   ignored.

5. If the line starts with an open square bracket (`[`), it indicates a section header (also known as configuration header).
   The line should contain a string with alphanumeric characters and underscores, indicating the header name, followed by a
   closing square bracket (`]`), to end the header. After any number of ignored whitespace characters there could be a `#`
   character. If this is the case, the rest of the line is handled as specified in point 3. Otherwise there should not be
   any other character (except the whitespace) on the line. Any line that does not comply to these specifications should
   lead to an immediate parse error. Multiple section headers with the same name are allowed. All key-value pairs following
   this section header are part of this section until a new section header is started.

6. If the line starts with an alphanumeric character, the line should indicate a key-value pair. The beginning of the line
   should contain a string of alphabetic characters, numbers, dots (`.`), colons (`:`) and underscores (`_`), but it may
   only start with an alphanumeric character. This string indicates the key. After an optional number of ignored whitespace,
   the key should be followed by an equality sign (`=`). Any text between the `=` and the first `#` character not enclosed
   within a pair of single or double quotes (`'` or `"`) is known as the non-stripped string. Any character after the `#` is
   handled as specified in point 3. If the line does not contain any non-enclosed `#` character, the value ends at the end
   of the line instead. The 'value' of the key-value pair is the non-stripped string with all whitespace in front and at the
   end stripped. The value may not be empty. Any line that does not comply to these specifications should lead to an
   immediate parse error.

7. The value may consist of multiple nested dimensions which are grouped by pairs of square brackets (`[` and `]`). The
   number of square brackets should be properly balanced, otherwise an error is raised. Square brackets which should not be
   used for grouping should be enclosed in quotation marks. Every dimension is split at every whitespace sequence and comma
   character (`,`) not enclosed in quotation marks. Implicit square brackets are added to the begin and end of the value, if
   these are not explicitly added. A few situations require explicit addition of outer brackets such as matrices with only
   one column element, i.e. with dimension 1xN.

8. The sections of the value which are interpreted as separate entities are named elements. For a single value the element
   is on the zeroth dimension, for arrays on the first dimension and for matrices on the second dimension. Elements can be
   forced by using quotation marks, either single or double quotes (`'` or `"`). The number of both types of quotation marks
   should be properly balanced, otherwise an error is raised. The conversion to the elements to the actual type is performed
   when accessing the value.

9. All key-value pairs defined before the first section header are part of a zero-length empty section header.

## Accessing parameters

Values are accessed via the configuration object. In the following example, the key is a string called `key`, the object is
named `config` and the type `TYPE` is a valid C++ type the value should represent. The values can be accessed via the
following methods:

```cpp
// Returns true if the key exists and false otherwise
config.has("key")
// Returns the number of keys found from the provided initializer list:
config.count({"key1", "key2", "key3"});
// Returns the value in the given type, throws an exception if not existing or a conversion to TYPE is not possible
config.get<TYPE>("key")
// Returns the value in the given type or the provided default value if it does not exist
config.get<TYPE>("key", default_value)
// Returns an array of elements of the given type
config.getArray<TYPE>("key")
// Returns a matrix: an array of arrays of elements of the given type
config.getMatrix<TYPE>("key")
// Returns an absolute (canonical if it should exist) path to a file
config.getPath("key", true /* check if path exists */)
// Return an array of absolute paths
config.getPathArray("key", false /* do not check if paths exists */)
// Returns the value as literal text including possible quotation marks
config.getText("key")
// Set the value of key to the default value if the key is not defined
config.setDefault("key", default_value)
// Set the value of the key to the default array if key is not defined
config.setDefaultArray<TYPE>("key", vector_of_default_values)
// Create an alias named new_key for the already existing old_key or throws an exception if the old_key does not exist
config.setAlias("new_key", "old_key")
```

Conversions to the requested type are using the `from_string` and `to_string` methods provided by the string utility library
described in [Section 4.8](./08_logging.md#internal-utilities). These conversions largely follow standard parsing, with one
important exception. If (and only if) the value is retrieved as a C/C++ string and the string is fully enclosed by a pair of
`"` characters, these are stripped before returning the value. Strings can thus also be provided with or without quotation
marks.

{{% alert title="Warning" color="warning" %}}
It should be noted that a conversion from string to the requested type is a comparatively heavy operation. For
performance-critical sections of the code, one should consider fetching the configuration value once and caching it in a
local variable.
{{% /alert %}}


[@tomlgit]: https://github.com/toml-lang/toml
