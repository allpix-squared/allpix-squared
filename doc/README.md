---
# SPDX-FileCopyrightText: 2022-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: CC-BY-4.0
---

# Allpix Squared Documentation

This file gives a few hints for editing the Markdown documentation of Allpix Squared.

The used Markdown specification is [GLFM](https://docs.gitlab.com/ee/user/markdown.html), so that the files can be nicely
display in GitLab's web interface. Since the main documentation deploy is via [hugo](https://gohugo.io/) using
[Docsy](https://www.docsy.dev/), files are annotated with YAML front matter including the relevant information for hugo.

[[_TOC_]]

## File tree

Top-level folder in the [usermanual folder](./usermanual/) correspond to chapters. The Markdown file corresponding to such a
folder is always the `_index.md` file in that folder. For example, for [`01_introduction/`](./usermanual/01_introduction/)
the corresponding chapter start is [`01_introduction/_index.md`](./usermanual/01_introduction/_index.md).

Markdown files in a chapter folder correspond to sections. Subsections and deeper subsectioning levels correspond to
[H2+ headings](https://docs.gitlab.com/ee/user/markdown.html#headers) in these files.

Although hugo supports giving chapters and sections numbers without sorting them by renaming, files in this documentation
will begin with their chapter / section number (always double-digit). This allows to browse the manual in the correct order
in GitLab's webinterface.

## Citations

Citations are done with Markdown's reference-style links, with additional (escaped) square brackets around. All reference
links should be at the bottom of the Markdown file in which they are included, separated by two empty spaces. To make them
play nicely with the LaTeX bibliography, the reference label has to begin with an `@`. For Example:

```markdown
This is a citation for the GLFM documentation about references and links \[[@glfm-links]\].


[@glfm-links]: https://docs.gitlab.com/ee/user/markdown.html#links
```

When adding or changing a citation, make sure to keep all changes in sync with the [references.bib](./latex/references.bib).
When changing the label/link of a reference, this includes making sure that reference is consistent between different Markdown
files (project-wide search is your friend).

If possible, reference links should always point to the DOI. A citation without a link is not possible.

## References

References to other parts of the documentation is possible via relative paths. For example, to reference
[`02_installation/_index.md`](./usermanual/02_installation/_index.md) in
[`01_introduction/_index.md`](./usermanual/01_introduction/_index.md), use

```markdown
This is a reference to [Chapter 2](../02_installation/_index.md) in `01_introduction/_index.md`.
```

To reference to another section in the same chapter, use `[Section YY.XX](./XX_some_section.md)`. References to headings
within a section can also be done using [Header IDs](https://docs.gitlab.com/ee/user/markdown.html#header-ids-and-links).

**Important**: *always* references inline and *always* start the path with at least one `.`, even if not necessary in GLFM.
The reason is that the paths need to be adjusted for hugo. During the conversion of the Markdown files it is assumed that
these two conditions are met.

## Renaming files and headings

If a file is renamed, for example because the section number changed, it needs to be made sure that all references to that
file are adjusted. The same applies when changing headings, as these can also be referenced via their Header IDs (see
[References](#references)). When changing a chapter/section number, don't forget to update the reference name as well, Again,
project-wide search is your friend here.

## Front matter

A typical YAML front-matter is

```yaml
title: "Title"
description: "Description"
weight: 1
```

The following notes should be taken for these fields:

-   `title` is always required displayed in the ToC / sidebar and as main heading of the doc page. For non-weighted markdown
    files, the filename should be a lower-cased version of the `title` with spaces replaced by underscores.

-   `description` is optional and always disaplyed below the `title`. In this documentation, it should only be present for
    chapter pages, modules and examples.

-   `weight` is required for all files in the [usermanual folder](./usermanual/). Files always should have weight as
    double-digit prefixed to their file name (see [File tree](#file-tree)). For files that are not in the usermanual folder,
    for example module READMEs, the `weight` field must be omitted so that hugo sorts them alphabetically.

For modules, additional front matter tags have to be set, as explained in the
[documentation](./usermanual/10_development/03_new_module.md#readmemd).

## Formulae

Refer to the [GLFM documentation](https://docs.gitlab.com/ee/user/markdown.html#math).

Note: since hugo does not support GitLab's math extension, these needs to be converted to some other format that is not using
backticks, i.e. [pandoc's tex_math_dollars format](https://pandoc.org/MANUAL.html#extension-tex_math_dollars). However, this
also means that explicit backticks (like `` $` `` or `` ```math ``) can't be used (e.g. to explain the GitLab's math mode).

## Testing the website

You can easily test the website of the documentation with your changes yourself by running:
```shell
git clone https://gitlab.cern.ch/allpix-squared/allpix-squared-website.git
./get_artifacts.sh job job_number_from_your_ci your_cern_user_name/allpix-squared
hugo server
```

For more information refer to [the README](https://gitlab.cern.ch/allpix-squared/allpix-squared-website#commands) in the
website repository.
