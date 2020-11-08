import lxml.etree as etree

# Check if we got token:
if len(sys.argv) != 3:
  sys.exit(1)

input_xml = sys.argv[1]
output_xml = sys.argv[1]

print "Reading XML input file"
dom = etree.parse(input_xml)

print "Reading XSLT file"
xslt = etree.parse("ctest-to-junit.xsl")

print "Transforming DOM"
transform = etree.XSLT(xslt)
newdom = transform(dom)

print "Writing XML output file"
newdom.write(output_xml, pretty_print=True)
