#!/bin/bash

# SPDX-FileCopyrightText: 2018-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

#Get the line for the CVMFS status and check if server is transaction
clicdp_status=`cvmfs_server list | grep clicdp`
if [[ $clicdp_status == *"(stratum0 / S3)"* ]]; then
  echo "No ongoing CVMFS transaction detected, initializing new transaction."
  # Start transaction
  cvmfs_server transaction clicdp.cern.ch

  # Deploy the latest Allpix Squared build
  echo "Deploying build: $2"
  if [[ $2 =~ v(.+?) ]]; then
    tag=${BASH_REMATCH[1]}
  else
    tag=$2
  fi
  cd /home/cvclicdp/

  # Extract artifact tars
  echo "Extract artifact tarballs"
  for filename in $1/*.tar.gz; do
    echo " - $filename"

    if [[ $filename =~ squared-(.+?)-x ]]; then
      version=${BASH_REMATCH[1]}
      if [ "$version" != "$tag" ]; then
        echo "Build version does not match: $version"
        continue
      fi
    else
      echo "Unable to parse string $filename"
      continue
    fi

    if [[ $filename =~ $version-(.+?)\.tar ]]; then
      flavor=${BASH_REMATCH[1]}
      echo "Found build flavor:  $flavor"
    else
      echo "Unable to parse string $filename"
      continue
    fi

    # Create target directory and untar:
    mkdir -p /home/cvclicdp/release/$version/$flavor
    tar --strip-components=1 -xf $filename -C /home/cvclicdp/release/$version/$flavor/
  done

  # Check if we found any version and flavor to be installed:
  if [[ -z $version || -z $flavor ]]; then
    echo "Did not find suitable version or flavor to install."
    echo "Aborting CVMFS transaction."
    cvmfs_server abort -f clicdp.cern.ch
    exit 1
  fi

  # Deleting old nightly build if it is not a tag
  if [ "$tag" == "latest" ]; then
    echo "Deleting old \"latest\" build"
    rm -rf /cvmfs/clicdp.cern.ch/software/allpix-squared/latest
  fi

  # Move new build into place
  echo "Moving new build into place"
  mv /home/cvclicdp/release/$tag /cvmfs/clicdp.cern.ch/software/allpix-squared/

  # Clean up old stuff
  rm -rf /home/cvclicdp/release

  # Publish changes
  echo "Publishing CVMFS transaction."
  cvmfs_server publish clicdp.cern.ch
  exit 0
else
  (>&2 echo "#################################")
  (>&2 echo "### CVMFS Transaction ongoing ###")
  (>&2 echo "######## Deploy cancelled #######")
  (>&2 echo "#################################")
  exit 1
fi
