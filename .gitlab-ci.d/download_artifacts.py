#!/usr/bin/env python

import os
import json

with open('pipeline_info.json') as json_data:
  data = json.load(json_data)

#Find last passed GCC build
for i in range(0, len(data)):
  if data[i]['name'] == 'pkg:cc7-gcc':
    print "Downloading CC7 GCC build"
    os.system("curl -O https://gitlab.cern.ch/allpix-squared/allpix-squared/-/jobs/%s/artifacts/raw/allpix-squared_cc7.tar" % data[i]['id'] )
    break

#Find last passed GCC build
for i in range(0, len(data)):
    if data[i]['name'] == 'pkg:slc6-gcc':
        print "Downloading SLC6 GCC build"
    os.system("curl -O https://gitlab.cern.ch/allpix-squared/allpix-squared/-/jobs/%s/artifacts/raw/allpix-squared_slc6.tar" % data[i]['id'] )
    break
