---
# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
title: "Interpretation of Pass and Fail Conditions"
weight: 3
---

Multiple pass or fail conditions can be separated by a semicolon or by adding multiple `#PASS` or `#FAIL` expressions. It
should however be noted that test passes or fails *if any of these conditions is met*, i.e. the conditions are combined with
a logical `OR`. At least one pass or one fail conditions must be present in every test.

Pass and fail condition are not interpreted as regular expressions but relevant characters are automatically escaped. This
allows to directly copy corresponding lines form the log into the respective condition without manually creating a matching
regular expression. A noteworthy exception to this are line breaks. To ease matching of multi-line expressions, the newline
escape sequence `\n` of any test expression is automatically expanded to `[\r\n\t ]*` to match any new line, carriage return,
tab and whitespace characters following the line break.
