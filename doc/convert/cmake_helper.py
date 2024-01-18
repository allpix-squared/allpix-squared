#!/usr/bin/python3

# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

"""
Helper script for CMake to generate targets for the documentation.
"""

import argparse
import glob
import os
import re
import shutil
from typing import Callable

import convert_markdown


def parse_cmdline_args(args: list[str] = None) -> argparse.Namespace:
    """
    Parses the command-line arguments.

    Args:
        args: Optional list of strings to parse as command-line arguments. If set to None, parse from sys.argv instead.

    Returns:
        A namespace with the parsed arguments.
    """
    parser = argparse.ArgumentParser(description='Allpix Squared Documentation CMake helper',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('function', choices=['copyonly',
                                             'copylower',
                                             'glob_readme',
                                             'glob_dir',
                                             'convert_hugo',
                                             'convert_latex',
                                             'create_latex_input',
                                             ])
    parser.add_argument('input_path', type=str, help='input path parameter')
    parser.add_argument('output_path', type=str, help='output path parameter')

    return parser.parse_args(args=args)


def _create_base_folder(file_path: str) -> None:
    """
    Creates the necessary folder to create a file.

    Args:
        file_path: Path of the file.
    """
    os.makedirs(os.path.dirname(file_path), exist_ok=True)


def copyonly(input_file: str, output_file: str) -> None:
    """
    Copy file from one location to another.

    Args:
        input_file: Path of the input file.
        output_file: Path to copy the file to.
    """
    _create_base_folder(output_file)
    shutil.copyfile(input_file, output_file)


def copylower(input_file: str, output_dir: str) -> None:
    """
    Copy file from one location to another, while using the converting the filename to lowercase.

    Args:
        input_file: Path of the input file.
        output_dir: Path of the directory to copy the file to.
    """
    output_file = os.path.join(output_dir, os.path.basename(input_file).lower())
    copyonly(input_file, output_file)


def glob_readme(input_dir: str, output_dir: str) -> None:
    """
    Copy files matching the READMEs in Allpix Squared in lowercase.

    For example: CSADigitizer/README.md becomes csadigitizer.md, the path given should be <apsq_root>/src/modules.

    Args:
        input_dir: Path of the directory to search for READMEs in subfolders.
        output_dir: Path of the directory to copy the renamed READMEs to.
    """
    glob_str = os.path.join(input_dir, '*', 'README.md')
    input_files = glob.glob(glob_str)
    for input_file in input_files:
        new_file = re.sub(rf'.*{os.sep}(.*?){os.sep}README\.md', r'\1.md', input_file)
        new_file = new_file.lower()
        output_file = os.path.join(output_dir, new_file)
        copyonly(input_file, output_file)


def glob_dir(input_dir: str, output_dir: str) -> None:
    """
    Copy files from a directory (except with .in file extension) to another directory while keeping the folder structure.

    Args:
        input_dir: Path of the directory to search for the files.
        output_dir: Path of the directory to copy the files to.
    """
    glob_str = os.path.join(input_dir, '**', '*.*[!.in]')
    input_files = glob.glob(glob_str, recursive=True)
    for input_file in input_files:
        relpath = os.path.relpath(input_file, input_dir)
        output_file = os.path.join(output_dir, relpath)
        copyonly(input_file, output_file)


def _convert_helper(input_dir: str, output_dir: str, md_converter: str, md_fname_converter: Callable[[str], str]) -> None:
    """
    Helper function to copy plain GLFM file tree to converted file tree.

    Args:
        input_dir: Path of the directory containing the GLFM tree.
        output_dir: Path of the directory to copy the converted files to.
        md_converter: CLI argument for the converter of convert_markdown.
        md_fname_converter: String-modifying function to adjust output file name for .md files.
    """
    for root_path, _dir_names, file_names in os.walk(input_dir):
        for file_name in file_names:
            input_file = os.path.join(root_path, file_name)
            relpath = os.path.relpath(input_file, input_dir)
            output_file = os.path.join(output_dir, relpath)
            if file_name.endswith('.md'):
                output_file = md_fname_converter(output_file)
                _create_base_folder(output_file)
                convert_markdown.main(args=[md_converter, input_file, output_file])
            else:
                copyonly(input_file, output_file)


def convert_hugo(input_dir: str, output_dir: str) -> None:
    """
    Copy plain GLFM file tree to hugo-adjusted file tree.

    Args:
        input_dir: Path of the directory containing the GLFM tree.
        output_dir: Path of the directory to copy the converted files to.
    """
    _convert_helper(input_dir, output_dir, 'hugo', lambda fname: fname)


def convert_latex(input_dir: str, output_dir: str) -> None:
    """
    Copy plain GLFM file tree to LaTeX file tree.

    Args:
        input_dir: Path of the directory containing the GLFM tree.
        output_dir: Path of the directory to copy the converted files to.
    """
    _convert_helper(input_dir, output_dir, 'latex', lambda fname: fname[:-3]+'.tex')


def create_latex_input(input_dir: str, output_file: str) -> None:
    """
    Creates a LaTeX file that includes all the .tex files in a given directory, sorted alphabetically.

    Args:
        input_dir: Path of the directory containing the LaTeX tree.
        output_file: Path of the file to copy the LaTeX input tree to.
    """
    latex_files = list[str]()
    for root_path, _dir_names, file_names in os.walk(input_dir):
        for file_name in file_names:
            file_path = os.path.join(root_path, file_name)
            if file_path.endswith('.tex'):
                latex_files.append(file_path)
    latex_files = sorted(latex_files, key=lambda x: x[:-10]+'00' if x.endswith('_index.tex') else x)
    _create_base_folder(output_file)
    with open(output_file, 'wt', encoding='utf-8') as output_file_stream:
        for latex_file in latex_files:
            output_file_stream.write(f'\\input{{{latex_file}}}\n')


def main(args: list[str] = None) -> None:
    """
    Main function.

    Args:
        args: Optional list of strings to parse as command-line arguments. If set to None, parse from sys.argv instead.
    """
    options = parse_cmdline_args(args=args)

    function_mapping = dict({
        'copyonly': copyonly,
        'copylower': copylower,
        'glob_readme': glob_readme,
        'glob_dir': glob_dir,
        'convert_hugo': convert_hugo,
        'convert_latex': convert_latex,
        'create_latex_input': create_latex_input,
    })

    function_mapping[options.function](options.input_path, options.output_path)


if __name__ == '__main__':
    main()
