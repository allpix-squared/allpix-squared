#!/usr/bin/python3

# SPDX-FileCopyrightText: 2022-2023 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

"""
Convert GitLab Flavored Markdown to hugo or LaTeX using pandoc.
"""

import argparse
import functools
import os
import pathlib
import re
import subprocess
import sys
import tempfile

import yaml


def parse_cmdline_args(args: list[str] = None) -> argparse.Namespace:
    """
    Parses the command-line arguments.

    Args:
        args: Optional list of strings to parse as command-line arguments. If set to None, parse from sys.argv instead.

    Returns:
        A namespace with the parsed arguments.
    """
    parser = argparse.ArgumentParser(description='Convert GitLab Flavored Markdown to hugo or LaTeX using pandoc.',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--isindexmd', choices=['auto', 'true', 'false'], default='auto', help='force if file is _index.md')
    parser.add_argument('converter', choices=['hugo', 'pandoc', 'latex'], default='hugo', help='GitLab Markdown converter')
    parser.add_argument('infile', type=argparse.FileType('r'), help='input file')
    parser.add_argument('outfile', type=argparse.FileType('w'), nargs='?', default=sys.stdout, help='output file')

    return parser.parse_args(args=args)


def hugo_convert_relative_paths(string: str) -> str:
    """
    Converts relative paths for hugo.

    hugo creates subfolders for each Markdown that is not _index.md, this means that the normal relative paths do not work
    and need to be adjusted. Looks complicated, but if you use a regex visualizer like debuggex.com, it's easy to understand.

    Args:
        string: String formatted in Markdown.

    Returns:
        String formatted in Markdown.
    """
    return re.sub(r'(\[.*?\]\()(\..+?)(\))', r'\1../\2\3', string)


def hugo_reference_remove_md(string: str) -> str:
    """
    Removes the .md file extension in references for hugo.

    hugo can handle Markdown references (`[]()`), but the file extension needs to be removed for them to work.
    In the special case of referencing an _index.md, the entire file needs to removed from the path.
    Looks complicated, but if you use a regex visualizer like debuggex.com, it's easy to understand.

    Args:
        string: String formatted in Markdown.

    Returns:
        String formatted in Markdown.
    """
    return re.sub(r'(\[.*?\]\()(\..+?)(/_index)?(\.md)(#.+?)?(\))', r'\1\2\5\6', string)


def hugo_front_matter_convert_pandoc(string: str) -> str:
    """
    Converts YAML front-matter in a string from hugo Markdown to plain text in Markdown.

    Args:
        string: String formatted in Markdown with YAML front-matter.

    Returns:
        String formatted in Markdown without YAML front-matter.
    """
    # extract yaml from string
    yaml_match = re.match(r'^---\n(.+?)\n---\n', string, flags=re.DOTALL)

    if yaml_match:
        raw_yaml = yaml_match.group(1)
        yaml_data = yaml.safe_load(raw_yaml)
        yaml_endpos = yaml_match.end(0)
        string_after_yaml = string[yaml_endpos:]

        converted_front_matter = '# ' + yaml_data['title'] + '\n'

        # convert module data, check for `module_status` in front matter
        if 'module_status' in yaml_data.keys():
            # we are in a module, prepare variables for table
            module_data = {'Status': yaml_data['module_status']}
            max_tabentry_length = len(yaml_data['module_status'])
            # fetch arrays
            for key in ['module_maintainers', 'module_inputs', 'module_outputs']:
                if key in yaml_data.keys():
                    module_data[key[7].upper() + key[8:]] = yaml_data[key]
                    for element in yaml_data[key]:
                        max_tabentry_length = max(max_tabentry_length, len(element))
            # table width formatting
            table_enclosing_row = ('{:-<12} {:-<' + str(max_tabentry_length) + '}').format('', '')
            table_format_string = ('{: <12} {: <' + str(max_tabentry_length) + '}')
            # create table
            converted_front_matter += '\n' + table_enclosing_row + '\n'
            for key, value in module_data.items():
                if isinstance(value, list):
                    # if list, put entries in separate lines with key only in first line
                    converted_front_matter += table_format_string.format(key, value[0]) + '\n'
                    for count in range(1, len(value)):
                        converted_front_matter += table_format_string.format('', value[count]) + '\n'
                else:
                    converted_front_matter += table_format_string.format(key, value) + '\n'
            converted_front_matter += table_enclosing_row

        string = converted_front_matter + string_after_yaml

    return string


def latex_fix_images(string: str, file_path: str) -> str:
    """
    Fixes the path and the image size in LaTeX.

    Args:
        string: String formatted in LaTeX.
        file_path: Path to the file where the string was extracted from.

    Returns:
        String formatted in LaTeX.
    """
    base_dir = os.path.dirname(file_path)
    return re.sub(r'\\includegraphics{\./(.+?)}',
                  rf'\\includegraphics[width=\\textwidth,height=0.6\\textheight,keepaspectratio]{{{base_dir}/\1}}',
                  string)


def latex_fix_duplicate_autocites(string: str) -> str:
    """
    Fixes duplicate cites created from the Markdown to LaTeX conversion.

    Args:
        string: String formatted in LaTeX.

    Returns:
        String formatted in LaTeX.
    """
    return re.sub(r'\\autocite{[^{} :]+?}:[ \n]\S+', '', string)


def latex_convert_hugo_alert(string: str) -> str:
    """
    Converts the pandoc-converted hugo alters to LaTeX warning boxes.

    Looks more complicated than it actually is: \s+ is supposed to be a space in most cases, but edge cases happen when
    pandoc does a linebreak where the space is.

    Args:
        string: String formatted in LaTeX.

    Returns:
        String formatted in LaTeX.
    """
    match = r'\\{\\{\\%\s+alert\s+title=``(.+?)\'\'\s+color=``(.+?)\'\'\s+\\%\\}\\}\s+(.+?)\s+\\{\\{\\%\s+/alert\s+\\%\\}\\}'
    return re.sub(match, r'\\begin{hugo\2}\n\\textbf{\1}:\n\3\n\\end{hugo\2}', string, flags=re.DOTALL)


def _get_path_relative_to_markdown_tree(file_path: str) -> str:
    """
    Return the path relative to the markdown tree (assuming at most one directory depth).

    For example, `/path/to/docs/01_introduction/_index.md` becomes `01_introduction/_index.md`.

    Args:
        file_path: Path to the file.

    Returns:
        String with the relative path.
    """
    file_path_pl = pathlib.Path(file_path).resolve()
    docs_dir_pl = file_path_pl.parents[1]
    file_path_rel_pl = file_path_pl.relative_to(docs_dir_pl)
    return file_path_rel_pl.as_posix()


def latex_convert_href_references(string: str, file_path: str) -> str:
    """
    Converts the pandoc-converted Markdown references to LaTeX references.

    Essentially, pandoc treats `[sometext](./some/path)` as href, so it's relatively easy to convert them to LaTeX refs.

    Args:
        string: String formatted in LaTeX.
        file_path: Path to the file where the string was extracted from.

    Returns:
        String formatted in LaTeX.
    """
    # Replace hypertargets with something that include part of the file path to prevent duplicate targets
    file_label_prefix = _get_path_relative_to_markdown_tree(file_path)
    # For the first target we don't want to include the section name as we don't have that in GLFM
    string = re.sub(r'^(\\hypertarget{)([^\n]+?)(}{%\n.+?\\label{)(.+?)(}})',
                    rf'\g<1>{file_label_prefix}\g<3>{file_label_prefix}\g<5>',
                    string, count=1, flags=re.DOTALL)
    # For the following targets we do want the section name to be included
    string = re.sub(r'(.+?\\hypertarget{)([^\n]+?)(}{%\n.+?\\label{)(.+?)(}})',
                    rf'\g<1>{file_label_prefix}-\g<2>\g<3>{file_label_prefix}-\g<4>\g<5>',
                    string, flags=re.DOTALL)
    # We need to adjust references in the same file as well though
    string = re.sub(r'(\\hyperlink{)(.+?)(}{)', rf'\g<1>{file_label_prefix}-\g<2>\g<3>', string)
    # Find references to other files
    refs = re.findall(r'(\\href{)(\.\.?/.+?)(})', string)
    for ref in refs:
        ref_rel_file_path = ref[1]
        ref_full_match = ref[0] + ref[1] + ref[2]
        ref_file_path = os.path.join(os.path.dirname(file_path), ref_rel_file_path)
        ref_replace = _get_path_relative_to_markdown_tree(ref_file_path).replace('\\#', '-')
        string = re.sub(re.escape(ref_full_match), rf'\\protect\\hyperlink{{{ref_replace}}}', string)

    return string


def gitlab2hugo(string: str, isindexmd: bool) -> str:
    """
    Converts a string from GitLab Markdown to hugo Markdown. Equations are formatted to pandoc's tex_math_dollars format.

    Args:
        string: String formatted in GitLab Markdown.
        isindexmd: If true the file where string is extracted from is an index page for hugo.

    Returns:
        String formatted in hugo Markdown.
    """
    if not isindexmd:
        string = hugo_convert_relative_paths(string)
    return hugo_reference_remove_md(string)


def gitlab2pandoc(string: str) -> str:
    """
    Converts a string from GitLab Markdown to pandoc Markdown. Only changes front matter.

    Args:
        string: String formatted in GitLab Markdown.

    Returns:
        String formatted in pandoc Markdown.
    """
    return hugo_front_matter_convert_pandoc(string)


def pandoc2latex(string: str, file_path: str, extra_args: list[str] = None) -> str:
    """
    Converts a string from pandoc Markdown to LaTeX.

    Args:
        string: String formatted in pandoc Markdown.
        file_path: Path to the file where the string was extracted from.
        extra_args: List of additional arguments for pandoc.

    Returns:
        String formatted in LaTeX.
    """
    # write string to temporary file for pandoc, not possible to convert from string
    tmp = tempfile.NamedTemporaryFile(mode='wt', encoding='utf-8', delete=False)  # pylint: disable=consider-using-with
    tmp.write(string)
    tmp.close()

    pandoc_args = [
        'pandoc',
        '-f', 'markdown+escaped_line_breaks+shortcut_reference_links+autolink_bare_uris+multiline_tables+raw_html-raw_tex',
        '-t', 'latex',
        '--listings', '--biblatex',
        '--lua-filter', 'pandoc-gitlab-math.lua',
        '--lua-filter', 'pandoc-minted.lua',
    ]
    if extra_args is not None:
        pandoc_args += extra_args
    pandoc_args += [tmp.name]

    proc = subprocess.run(pandoc_args, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
    string = proc.stdout

    os.unlink(tmp.name)

    string = latex_fix_images(string, file_path)
    string = latex_fix_duplicate_autocites(string)
    string = latex_convert_hugo_alert(string)
    string = latex_convert_href_references(string, file_path)

    return string


def gitlab2latex(string: str, isindexmd: bool, file_path: str) -> str:
    """
    Converts a string from GitLab Markdown to LaTeX.

    Args:
        string: String formatted in GitLab Markdown.
        isindexmd: If true, the top level division will be set to chapter.
        file_path: Path to the file where the string was extracted from.

    Returns:
        String formatted in LaTeX.
    """
    pandoc_extra_args = list[str]()

    if isindexmd:
        pandoc_extra_args.append('--top-level-division=chapter')

    return pandoc2latex(gitlab2pandoc(string), file_path, pandoc_extra_args)


def main(args: list[str] = None) -> None:
    """
    Main function.

    Args:
        args: Optional list of strings to parse as command-line arguments. If set to None, parse from sys.argv instead.
    """
    options = parse_cmdline_args(args=args)

    file_path = options.infile.name
    file_content = options.infile.read()

    isindexmd = False
    if options.isindexmd == 'auto':
        if file_path.endswith('_index.md'):
            isindexmd = True
    elif options.isindexmd == 'true':
        isindexmd = True

    function_mapping = dict({
        'hugo': functools.partial(gitlab2hugo, isindexmd=isindexmd),
        'pandoc': gitlab2pandoc,
        'latex': functools.partial(gitlab2latex, isindexmd=isindexmd, file_path=file_path),
    })

    new_file_content = function_mapping[options.converter](file_content)

    options.outfile.write(new_file_content)


if __name__ == '__main__':
    main()
