#!/usr/bin/env python

import sys
import lxml.etree as et

# Check if we got token:
if len(sys.argv) != 3:
  sys.exit(1)

input_xml = sys.argv[1]
input_xsl = sys.argv[2]
output_xml = sys.argv[3]

print("Reading XML input file")
dom = et.parse(input_xml)

print("Reading XSLT file")
xslt = et.parse(input_xsl)

print("Transforming DOM")
transform = et.XSLT(xslt)
newdom = transform(dom)

print("Writing XML output file")
newdom.write(output_xml, pretty_print=True)
