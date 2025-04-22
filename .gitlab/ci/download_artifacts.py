#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2018-2025 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

import os
import json
import sys

# Check if we got token:
if len(sys.argv) != 4:
  sys.exit(1)

api_token = sys.argv[1]
ci_project_id = sys.argv[2]
ci_pipeline_id = sys.argv[3]

print("Downloading job list")
data = json.load(os.popen("curl -g --header \"PRIVATE-TOKEN:%s\" \"https://gitlab.cern.ch/api/v4/projects/%s/pipelines/%s/jobs?scope[]=success&per_page=100\"" % (api_token, ci_project_id, ci_pipeline_id)))

# Find last passed builds
for i in range(0, len(data)):
  if "pkg:" not in data[i]['name']:
    continue

  print("Downloading artifact for job %s" % data[i]['id'])
  os.system("curl --location --header \"PRIVATE-TOKEN:%s\" \"https://gitlab.cern.ch/api/v4/projects/%s/jobs/%s/artifacts\" -o artifact.zip" % (api_token, ci_project_id, data[i]['id']))
  os.system("unzip -j artifact.zip; rm artifact.zip")
