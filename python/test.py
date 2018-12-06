#!/usr/bin/python

def findstringinline(line):
    tmp = line.split('(')[1].split(',')[3]
    print(tmp)
    tmp=tmp.strip(' ')
    tmp=tmp.strip(')')
    print(tmp)
    newline = line.replace(tmp, 'XXX')
    print(newline)

def delNULLstringInlines(lines):
    nullNumber=lines.count('NULL') 
    if (nullNumber == 3):
        newline=lines.strip('')
        newline = newline.replace('NULL,', '', 1)
    print newline
    return newline

def getModuleNameFromCommit(lines, lastModule):
    module = lastModule
    if 'Module Name****' in lines:
        module=lines.split('****')[1]
        print module
    return module


def replaceElementNodeArray(lines, lastModule):
    newline = lines.replace('element_node_parameter', 'check_range_node_parameter')
    tmpPattern = newline.split(' ')[2]
    print(tmpPattern)
    newline = newline.replace(tmpPattern, lastModule+'_range[]')
    print newline
    return newline


findstringinline("PARSE_XML_STRING(vsys_spec, vsys_name, struct xml_vsys_spec, XML_MAX_NAME_LEN)")
delNULLstringInlines('{"", NULL, NULL, NULL, TYPE_XML_INT}')
module = []
module = getModuleNameFromCommit('//Module Name****vsys_switch_ui****Range Check Element and Function BEGIN:', module)
replaceElementNodeArray('struct element_node_parameter vsys_switch_element_parameter[] =', module)
