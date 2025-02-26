#!/usr/bin/python3

# SPDX-FileCopyrightText: 2022-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

"""
Update copyright years for files in a git repo. The initial year is taken from git.
"""

import argparse
import logging
import os
import re
import subprocess
import time


DEFAULT_YEARS_PATTERN = r'([12]\d{3})(-[12]\d{3})?'


class MatchPattern:
    """
    Class containing functions for year matching and replacing.

    Attributes:
        match_pattern: Regex pattern for year matching. The first match group contains the substring to be replaced.
        replace_pattern: Function that returns the replace pattern for a given year range.
    """

    def __init__(self, years: str, appendix: str = None, prependix: str = None) -> None:
        self._match_pattern = ''
        self._prependix_repl = ''
        self._appendix_repl = ''

        # only add prependix with space if non-empty
        if prependix is not None:
            self._prependix_repl = prependix + ' '
            self._match_pattern += re.escape(prependix) + ' '

        # add top-level match group to year matching pattern
        self._match_pattern += rf'({years})'

        # only add appendix with space if non-empty
        if appendix is not None:
            self._appendix_repl = ' ' + appendix
            self._match_pattern += ' ' + re.escape(appendix)

    @property
    def match_pattern(self):
        """
        Regex pattern for year matching. The first match group contains the substring to be replaced.
        """
        return self._match_pattern

    def replace_pattern(self, years: str) -> str:
        """
        Returns the replace pattern for a given year range.

        Args:
            years: String to replace the year range with.

        Returns:
            String with the replace pattern.
        """
        return self._prependix_repl + years + self._appendix_repl


def parse_cmdline_args(args: list[str] = None) -> argparse.Namespace:
    """
    Parses the command-line arguments.

    Args:
        args: Optional list of strings to parse as command-line arguments. If set to None, parse from sys.argv instead.

    Returns:
        A namespace with the parsed arguments.
    """

    parser = argparse.ArgumentParser(description='Update copyright years for files in a git repo.')

    parser.add_argument('-v', '--verbose', help='enable verbose output', action='store_true')
    parser.add_argument('-a', '--appendix', help='pattern to append to year matching, i.e. <years> <pattern>', action='store', type=str)
    parser.add_argument('-p', '--prependix', help='pattern to prepend to year matching, i.e. <pattern> <years>', action='store', type=str)
    parser.add_argument('-y', '--years', help='regex pattern to match years', action='store', type=str, default=DEFAULT_YEARS_PATTERN)
    parser.add_argument('-x', '--exclude', help='files to exclude', action='extend', nargs='+', type=str, default=list[str]())
    parser.add_argument('-f', '--files', help='only run on specified files', action='extend', nargs='+', type=str, default=None)

    return parser.parse_args(args=args)


def get_known_file_paths_git() -> list[str]:
    """
    Get paths of all files known to git.

    Returns:
        A list containing strings with the relative paths of the files.
    """

    proc = subprocess.run(['git', 'ls-files'], check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')

    if proc.returncode != 0:
        raise Exception(f'git ls-files failed:\n{proc.stderr}')

    known_file_paths = proc.stdout.splitlines()

    return known_file_paths


def expand_dir_paths(paths: list[str]) -> list[str]:
    """
    Expand directories in a list of paths to explicit file paths.

    Args:
        paths: A list containing strings with paths.

    Returns:
        A list containing strings with paths that point to files.
    """

    file_paths = list[str]()

    for path in paths:
        if os.path.isdir(path):
            for root_dir, _dirs, files in os.walk(path):
                file_paths += [os.path.join(root_dir, file) for file in files]
        elif os.path.isfile(path):
            file_paths.append(path)

    return file_paths


def get_file_paths(file_paths: list[str], excluded_paths: list[str]) -> list[str]:
    """
    Get list of relaitve file paths from an include and exclude path list. If no included paths are specified, get a file list via git instead.

    Args:
        included_paths: A list containing strings with paths, can be None.
        excluded_paths: A list containing strings with paths.

    Returns:
        A list containing strings with relative paths.
    """

    # lambda to convert list of paths to relative paths
    def convert_to_relative_paths(paths: list[str]) -> list[str]:
        return [os.path.relpath(path) for path in paths]

    # get files from git except if file_paths is not specified
    if file_paths is None:
        logging.debug('Looking up all files known to git in %s', os.getcwd())
        # get files from git (relative paths)
        file_paths = get_known_file_paths_git()
    else:
        logging.debug('Using files specified via the include option')
        # ensure relative paths from user input
        file_paths = convert_to_relative_paths(file_paths)
        # expand dirs in file_paths (e.g. --files src/)
        file_paths = expand_dir_paths(file_paths)

    # ensure relative paths from user input
    excluded_paths = convert_to_relative_paths(excluded_paths)

    # lambda to check if to paths share a common prefix
    def has_commonprefix(path1: str, path2: str):
        return os.path.commonprefix([path1, path2]) != ''

    # remove excluded paths
    for file_path in file_paths.copy():
        for excluded_path in excluded_paths:
            if has_commonprefix(file_path, excluded_path):
                file_paths.remove(file_path)
                continue

    return file_paths


def get_current_year() -> str:
    """
    Get the current year in UTC (+0, no DST).

    Returns:
        String containing the current year.
    """

    current_year = time.strftime('%Y', time.gmtime())

    logging.debug('Setting current year to %s', current_year)

    return current_year


def replace_copyright_years(string: str, years: str, matching_opts: MatchPattern) -> tuple[str, str]:
    """
    Replace the first copyright year range in a string.

    Args:
        string: String in which to replace the copyright year range.
        years: String to replace the year range with.
        matching_opts: MatchingOptions with options for pattern matching.

    Returns:
        Tuple with two strings, the first is the given string with the replaced year range. The second entry is a string with the replaced year range that got
        replaced. If no year range was found, the first string return the given string unchanged and the second entry is none.
    """

    match_pattern = matching_opts.match_pattern
    replace_pattern = matching_opts.replace_pattern(years)

    # first only search for pattern to get replaced years, then replace it
    match = re.search(match_pattern, string, flags=re.MULTILINE)
    if not match:
        return (string, None)

    replaced_years = match[1]
    replaced_string = re.sub(match_pattern, replace_pattern, string, count=1, flags=re.MULTILINE)
    return (replaced_string, replaced_years)


def update_copyright_years(file_path: str, current_year: str, matching_opts: MatchPattern) -> bool:
    """
    Update the copyright year range in file from initial year to current year. The initial year is determined via git.

    Args:
        file_path: String containing the relative path.
        current_year: String containing the current year.
        matching_opts: MatchingOptions with options for pattern matching.

    Returns:
        Bool that is true if the copyright year range was changed.
    """

    # list all commits on file, including renames, and format to only output the year
    git_cmd = ['git', 'log', '--follow', '--format=%ad', '--date=format-local:%Y', file_path]

    # ensure dates are given in UTC (+0, no DST)
    os.environ['TZ'] = 'UTC0'

    proc = subprocess.run(git_cmd, check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')

    if proc.returncode != 0:
        raise Exception(f'git failed to find initial year for {file_path}:\n{proc.stderr}')

    if len(proc.stdout) == 0:
        logging.debug('git has no initial year information for %s, assuming current year', file_path)
        initial_year = current_year
    else:
        initial_year = proc.stdout.splitlines()[-1]

    years = initial_year
    if int(current_year) > int(initial_year):
        years += f'-{current_year}'

    # try to open as text file, skip if not possible
    file_content = str()
    try:
        with open(file_path, mode='rt', encoding='utf-8') as file:
            file_content = file.read()
    except UnicodeDecodeError:
        logging.debug('Skipped %s (not a text file)', file_path)
        return False

    replaced_file_content, replaced_years = replace_copyright_years(file_content, years, matching_opts)

    if replaced_years is None:
        logging.debug('Skipped %s (no year range found)', file_path)
        return False

    with open(file_path, mode='wt', encoding='utf-8') as file:
        file.write(replaced_file_content)

    logging.debug('Replaced copyright years from %s to %s in %s', replaced_years, years, file_path)
    return True


def main(args: list[str] = None) -> None:
    """
    Main loop adjusting all files in the current working directory.

    Args:
        args: Optional list of strings to parse as command-line arguments. If set to None, parse from sys.argv instead.
    """

    logging.basicConfig(level=logging.INFO, format='%(message)s')

    options = parse_cmdline_args(args=args)

    if options.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
        logging.debug('Running with verbose output')

    matching_opts = MatchPattern(options.years, options.appendix, options.prependix)

    file_paths = get_file_paths(options.files, options.exclude)

    current_year = get_current_year()

    logging.info('Looping over %d files, this might take a while', len(file_paths))

    counter = 0
    for file_path in file_paths:
        if update_copyright_years(file_path, current_year, matching_opts):
            counter += 1

    logging.info('Updated copyright years in %d files', counter)


if __name__ == '__main__':
    main()
