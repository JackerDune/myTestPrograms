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
    tmp = line.split('(')[1].split(',')[3]
    print(tmp)
    tmp.strip(' ') 

def replaceXML_PARSE(lines):
    if 'PARSE_XML_INT' in lines:
        lines.replace('PARSE_XML_INT', 'CHECK_XML_INT_INIT')
        lines.replace(')', 'XXX, XXX)')
    elif 'PARSE_XML_LONG' in lines:
        lines.replace('PARSE_XML_LONG', 'CHECK_XML_LONG_INIT')
        lines.replace(')', 'XXX, XXX)')
    elif 'PARSE_XML_ULONG' in lines:
        lines.replace('PARSE_XML_ULONG', 'CHECK_XML_ULONG_INIT')
        lines.replace(')', 'XXX, XXX)')
    elif 'PARSE_XML_STRING' in lines:
        lines.replace('PARSE_XML_STRING', 'CHECK_XML_STRING_INIT')
        lines.replace(')', 'XXX)')
    elif 'PARSE_XML_CHAR' in lines:
        lines.replace('PARSE_XML_STRING', 'CHECK_XML_CHAR_INIT')
    elif 'PARSE_XML_SPECIAL_STRING' in lines:
        lines.replace('PARSE_XML_STRING', 'CHECK_XML_STRING_INIT')
    elif 'PARSE_XML_NOLIMIT_STRING' in lines:
        lines.replace('PARSE_XML_NOLIMIT_STRING', 'CHECK_XML_STRING_INIT')
    elif 'PARSE_XML_INTERFACE' in lines:
        lines.replace('PARSE_XML_INTERFACE', 'CHECK_XML_STRING_INIT')
    elif 'PARSE_XML_IPMASK' in lines:
        lines.replace('PARSE_XML_IPMASK', 'CHECK_XML_STRING_INIT')
    elif 'PARSE_XML_IP' in lines:
        lines.replace('PARSE_XML_IP', 'CHECK_XML_STRING_INIT')
    elif 'PARSE_XML_IPNORMAL' in lines:
        lines.replace('PARSE_XML_IPNORMAL', 'CHECK_XML_STRING_INIT')

def processInputTmpFile(infile):
    tmpfd = open(tmpfile,'r')
    outfd = open(outfile,'w+')
    
    while True:
            lines = tmpfd.readline()
            if not lines:
                break
                pass
     


#=================================================================
def getxmlinfofromcfile(infile):
	"从C文件中查找以PARSE_XML_开头的和struct module_node_parameter开头的，且包含value的数据行，写入临时文件"
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
                            if linenumber == 5:
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
sys.setdefaultencoding('utf-8')
getxmlinfofromcfile('./xml_vrf.c')
