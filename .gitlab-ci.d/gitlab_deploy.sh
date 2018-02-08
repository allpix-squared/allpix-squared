#!/bin/bash

#Get the line for the CVMFS status and chech if server is transaction
clicdp_status=`cvmfs_server list | grep clicdp`
if [[ $clicdp_status == *"(stratum0 / local)"* ]]; then
  echo "I am not in transaction"
  # Start transaction
  #cvmfs_server transaction clicdp.cern.ch

  # Deploy the latest Allpix Squared build
  echo "Deploying build: $2"
  cd /home/cvclicdp/

  # Extract artifact tars
  echo "Extract the artifact .tar files"
  ls -l $1/
  mkdir -p /home/cvclicdp/artifact/cc7
  tar xf $1/allpix-squared_cc7.tar -C /home/cvclicdp/artifact/cc7
  mkdir -p /home/cvclicdp/release/$2/x86_64-centos7-gcc7-opt/
  tar xf /home/cvclicdp/artifact/cc7/build/allpix-squared-1.1.0-Linux.tar.gz -C /home/cvclicdp/release/$2/x86_64-centos7-gcc7-opt/

  mkdir -p /home/cvclicdp/artifact/slc6
  tar xf $1/allpix-squared_slc6.tar -C /home/cvclicdp/artifact/slc6
  mkdir -p /home/cvclicdp/release/$2/x86_64-slc6-gcc7-opt/
  tar xf /home/cvclicdp/artifact/slc6/build/allpix-squared-1.1.0-Linux.tar.gz -C /home/cvclicdp/release/$2/x86_64-slc6-gcc7-opt/

  #Deleting old nightly build if it is not a tag
  if [ "$2" == "nightly" ]; then
    echo "Deleting old nightly build"
    rm -rf /cvmfs/clicdp.cern.ch/software/allpix-squared/nightly
  fi

  # Move new build into place
  echo "Moving new build into place"
  # mv /home/cvclicdp/release/$2 /cvmfs/clicdp.cern.ch/software/allpix-squared/

  # Clean up old stuff
  rm -rf /home/cvclicdp/artifact
  rm -f /home/cvclicdp/release

  # Publish changes
  #cvmfs_server publish clicdp.cern.ch
  exit 0
else
  (>&2 echo "#################################")
  (>&2 echo "### CVMFS Transastion ongoing ###")
  (>&2 echo "### Nightly deploy cancelled  ###")
  (>&2 echo "#################################")
  exit 1
fi
