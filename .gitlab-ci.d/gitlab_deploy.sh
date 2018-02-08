#!/bin/bash

#Get the line for the CVMFS status and chech if server is transaction
clicdp_status=`cvmfs_server list | grep clicdp`
if [[ $clicdp_status == *"(stratum0 / local)"* ]]; then
  echo "I am not in transaction"
  # Start transaction
  cvmfs_server transaction clicdp.cern.ch

  # Deploy the latest Allpix Squared build
  echo "Deploying build: $2"
  cd /home/cvclicdp/

  # Extract artifact tars
  echo "Extract the artifact tarballs"
  for FLAVOUR in "x86_64-centos7-gcc7-opt" "x86_64-slc6-gcc7-opt"; do
    echo " - $FLAVOUR"
    mkdir -p /home/cvclicdp/release/$2/$FLAVOUR/
    tar --strip-components=1 -xf $1/allpix-squared-latest_$FLAVOUR.tar.gz -C /home/cvclicdp/release/$2/$FLAVOUR/
  done

  #Deleting old nightly build if it is not a tag
  if [ "$2" == "nightly" ]; then
    echo "Deleting old nightly build"
    rm -rf /cvmfs/clicdp.cern.ch/software/allpix-squared/nightly
  fi

  # Move new build into place
  echo "Moving new build into place"
  mv /home/cvclicdp/release/$2 /cvmfs/clicdp.cern.ch/software/allpix-squared/

  # Clean up old stuff
  rm -rf /home/cvclicdp/release

  # Publish changes
  cvmfs_server publish clicdp.cern.ch
  exit 0
else
  (>&2 echo "#################################")
  (>&2 echo "### CVMFS Transastion ongoing ###")
  (>&2 echo "### Nightly deploy cancelled  ###")
  (>&2 echo "#################################")
  exit 1
fi
