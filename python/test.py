#!/usr/bin/python

def findstringinline(line):
    tmp = line.split('(')[1].split(',')[3]
    print(tmp)
    tmp=tmp.strip(' ')
    tmp=tmp.strip(')')
    print(tmp)
    newline = line.replace(tmp, 'XXX')
    print(newline)

findstringinline("PARSE_XML_STRING(vsys_spec, vsys_name, struct xml_vsys_spec, XML_MAX_NAME_LEN)")
