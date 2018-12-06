#!/usr/bin/python
#-*- coding: utf-8 -*-

import sys, getopt
import os
import os.path
import chardet
import shutil

tmpfile = 'tmpStoreFile'
outfile = 'tmpOutFile'
rootdir = os.getcwd()
replacePattern = 'XXX'


def getModuleNameFromLines(lines):
#    print(lines)
    retString = lines.split("(")[1].split(",")[0]   
    return retString

def writeModuleCheckFunction(writeFd, module):
    moduleArrayString = module+'_range'
    writeFd.write('static inline int xml_'+ module + '_check_range(void *ptr)'+'\n')
    writeFd.write('{'+'\n')
    writeFd.write('    int i = 0;'+'\n')
    writeFd.write('    int ret = 0;'+'\n')
    writeFd.write('\n')
    writeFd.write('    for (i = 0; i< sizeof('+ moduleArrayString +')/sizeof(struct check_range_node_parameter); i++) {' + '\n')
    writeFd.write('        if ('+ moduleArrayString+ '[i].check_func != NULL) { ' + '\n')
    writeFd.write('            ret='+ moduleArrayString+ '[i].check_func((void *)ptr);' + '\n')
    writeFd.write('            if (ret == 0) {' + '\n')
    writeFd.write('                printf(\"ret is : %d, name is %s\\n\", ret,'+ moduleArrayString+ '[i].name);' + '\n')
    writeFd.write('                return ret;' + '\n')
    writeFd.write('            }' + '\n')
    writeFd.write('        }' + '\n')
    writeFd.write('    }' + '\n')
    writeFd.write('\n')
    writeFd.write('    return ret;' + '\n')
    writeFd.write('}'+'\n')

def replaceXML_PARSE_STRING_END(lines):
#    print(lines)
    tmp = lines.split('(')[1].split(',')[3]
    tmp=tmp.strip(' ')
#   print(tmp)
    tmp=tmp.strip(')')
#    print(tmp)
    newline = lines.replace(tmp, replacePattern + ')\n')
#    print(newline)
    return newline

def replaceXML_PARSE(lines):
    newline=lines
    if 'PARSE_XML_INT' in lines:
        newline=lines.replace('PARSE_XML_INT', 'CHECK_XML_INT_INIT')
        newline=newline.replace(')', ', '+replacePattern+', '+ replacePattern + ')')
    elif 'PARSE_XML_LONG' in lines:
        newline=lines.replace('PARSE_XML_LONG', 'CHECK_XML_LONG_INIT')
        newline=newline.replace(')', ', '+replacePattern+', '+ replacePattern + ')')
    elif 'PARSE_XML_ULONG' in lines:
        newline=lines.replace('PARSE_XML_ULONG', 'CHECK_XML_ULONG_INIT')
        newline=newline.replace(')', ', '+replacePattern+', '+ replacePattern + ')')
    elif 'PARSE_XML_STRING' in lines:
        newline=lines.replace('PARSE_XML_STRING', 'CHECK_XML_STRING_INIT')
        newline = replaceXML_PARSE_STRING_END(newline)
    elif 'PARSE_XML_STRUCT' in lines:
        newline=lines.replace('PARSE_XML_STRUCT', 'CHECK_XML_STRUCT_INIT_STRUCT')
        newline=newline.replace(')', ', '+replacePattern+')')
    elif 'PARSE_XML_CHAR' in lines:
        lines.replace('PARSE_XML_STRING', 'CHECK_XML_CHAR_INIT')
        newline=newline.replace(')', ', '+replacePattern+', '+ replacePattern + ')')
    elif 'PARSE_XML_SPECIAL_STRING' in lines:
        newline=lines.replace('PARSE_XML_STRING', 'CHECK_XML_STRING_INIT')
        newline = replaceXML_PARSE_STRING_END(newline)
    elif 'PARSE_XML_NOLIMIT_STRING' in lines:
        newline=lines.replace('PARSE_XML_STRING', 'CHECK_XML_STRING_INIT')
        newline = replaceXML_PARSE_STRING_END(newline)
    elif 'PARSE_XML_INTERFACE' in lines:
        newline=lines.replace('PARSE_XML_INTERFACE', 'CHECK_XML_STRING_INIT')
        newline=replaceXML_PARSE_STRING_END(newline)
    elif 'PARSE_XML_IPMASK' in lines:
        lines.replace('PARSE_XML_IPMASK', 'CHECK_XML_STRING_INIT')
    elif 'PARSE_XML_IP' in lines:
        newline=lines.replace('PARSE_XML_IP', 'CHECK_XML_STRING_INIT')
        newline=newline.replace(')', ', '+replacePattern+')')
    elif 'PARSE_XML_IPNORMAL' in lines:
        newline=lines.replace('PARSE_XML_IPNORMAL', 'CHECK_XML_STRING_INIT')
        newline=newline.replace(')', ', '+replacePattern+')')
    return newline

def replace_HASH_NODE_PARAM_ADD(lines):
    newline=lines
    if 'HASH_NODE_PARAM_ADD' in lines:
        newline = lines.replace('HASH_NODE_PARAM_ADD', 'HASH_RANGE_NODE_PARAM_ADD')
    
    return newline
    
def delNULLstringInlines(lines): #del last line's NULL string in element_node_parameter
    newline=lines
    nullNumber=lines.count('NULL') 
    if (nullNumber == 3):
        newline=lines.strip('')
        newline = newline.replace('NULL,', '', 2)
   # print newline
    return newline

def getModuleNameFromCommit(lines, lastModule):
    module = lastModule
    if 'Module Name****' in lines:
        module=lines.split('****')[1]
        #print module
    return module

def replaceElementNodeArray(lines, lastModule):
    newline = lines
    if 'element_node_parameter' in lines:
        newline = lines.replace('element_node_parameter', 'check_range_node_parameter')
        #print newline
        tmpPattern = newline.split(' ')[2]
        #print(tmpPattern)
        newline = newline.replace(tmpPattern, lastModule+'_range[]')
        #print newline
    return newline

    
def processInputTmpFile(infile):
    tmpfd = open(tmpfile,'r')
    outfd = open(outfile,'w+')
    module = []

    while True:
        lines = tmpfd.readline()
        if not lines:
            break
            pass
        newline = replaceXML_PARSE(lines)
        newline = replace_HASH_NODE_PARAM_ADD(newline)
        newline = delNULLstringInlines(newline)
        module=getModuleNameFromCommit(newline, module)
        newline=replaceElementNodeArray(newline, module)
        outfd.write(newline)
    tmpfd.close()
    outfd.close()

def processOutputFile(inputfile):
    insertIndex = 0
    with open(inputfile, 'r') as infd:
        with open(outfile, 'r') as processfd:
            with open(inputfile+'.new', 'w') as outfd:
                origin_lines = infd.readlines()
                processed_lines = processfd.readlines()
                for i in range(0, len(origin_lines)):
                    if 'PARSE_XML_' in  origin_lines[i]:
                        insertIndex = i;
                        break
                for i in range(0, len(origin_lines)):
                    outfd.write(origin_lines[i])
                    if i == insertIndex -1:
                        outfd.writelines(processed_lines) 
            
     

#=================================================================
def getxmlinfofromcfile(infile):
	"从C文件中查找以PARSE_XML_入临时文件"
	tmpfd = open(tmpfile,'w+')
	infd = open(infile,'r')
	write_flag = 0
	oplist = []
	lastModule = []
        linelist = []
        linenumber = 0;

	while True:
		lines = infd.readline()
		if not lines:
			break
			pass
		if 'PARSE_XML_' in lines:
                	module = getModuleNameFromLines(lines)
			#print(module)
            		if module != lastModule:
                		tmpfd.write('//Module Name****' + module + '****Range Check Element and Function BEGIN:' + "\n")
                		lastModule = module
            		tmpfd.write(lines)
		elif 'element_node_parameter' in lines:
                        linenumber=0
                        linelist.append(lines)	
                        while True:
                            linenumber = linenumber+1
                            if linenumber == 5: ###IF element node parameter 5 lines below has no HASH_NODE_PARAM_ADD, then pass
                                break
                            if not lines:
                                break
                                pass
                            if '};' in lines:
                                linelist = []
                                break

                            lines = infd.readline()
                            linelist.append(lines)
                            if 'HASH_NODE_PARAM_ADD' in lines: #if has This Element ,then we will replace something later
                                write_flag = 1
                                for member in linelist: 
                                    tmpfd.write(member)
                                    print(member)
                                linelist = [] #free linelist
                                break
		elif write_flag == 1:
			tmpfd.write(lines)
			if '};' in lines:
				write_flag = 0
                                writeModuleCheckFunction(tmpfd, module)
				tmpfd.write('//Module Name****' + module + '****Range Check Element and Function END:' + "\n\n\n")
	infd.close()
	tmpfd.close()
	return (oplist)

reload(sys)
fileName = sys.argv[1]
sys.setdefaultencoding('utf-8')
getxmlinfofromcfile(fileName)
processInputTmpFile(tmpfile)
processOutputFile(fileName)
