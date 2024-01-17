#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

##---

REPLACE=("pre-commit-clang-format" "pre-push-tag-version")
WARNING=("pre-commit" "pre-push")

SRCDIR=$(git rev-parse --show-toplevel)/etc/git-hooks
TGTDIR=$(readlink -f $(git rev-parse --git-dir))/hooks

##---

InstallHook () {
    cp $SRCDIR/$1-hook $TGTDIR/$1
    chmod +x $TGTDIR/$1
}


InstallReplaceHooks () {
    for HOOK in "${REPLACE[@]}"
    do
	if [ -f $TGTDIR/$HOOK ]
	then
	    echo "WARNING: File $TGTDIR/$HOOK already exists! Overwriting previous version..."
	fi
	InstallHook $HOOK
    done
}

InstallWarningHooks () {
    for HOOK in "${WARNING[@]}"
    do
	if [ -f $TGTDIR/$HOOK ]
	then
	    echo "ERROR: File $TGTDIR/$HOOK already exists! Will not overwrite, add script manually if needed."
	else
	    InstallHook $HOOK
	fi
    done
}

##---

InstallReplaceHooks
InstallWarningHooks
