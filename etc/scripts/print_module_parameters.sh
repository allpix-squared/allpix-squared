#!/bin/bash

# SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

VAR=""
if [ -z "$1" ]; then
    echo "Expecting path to module as argument."
    exit 0
fi

echo -e "\nParameters for module \"$(basename $1)\":\n"

FILES=$1/*

OUTPUT="NAME|TYPE\n---------------|------------------\n"
for f in $FILES
do
    while read -r line ; do
	#echo "Processing $line"
	name=$(echo "$line" | cut -d '"' -f2)
	type=$(echo "$line" | cut -d "<" -f2 | cut -d ">" -f1)
	OUTPUT="$OUTPUT\n$name|$type\n"
    done <<< "$(grep "config_.get" $f)"
done

echo -ne $OUTPUT | column -t -s "|"
