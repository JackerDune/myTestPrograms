#!/usr/bin/python
#-*- coding: utf-8 -*-

import sys, getopt
import os
import os.path
import chardet
import shutil

tmpfile = 'tmpaaa.txt'
rootdir = os.getcwd()
#=================================================================


def getxmlinfofromcfile(infile, value):
	"从C文件中查找以PARSE_XML_开头的和struct module_node_parameter开头的，且包含value的数据行，写入临时文件"
	tmpfd = open(tmpfile,'w+')
	infd = open(infile,'r')
	flag = 0
	oplist = []
	
	while True:
		lines = infd.readline()
		if not lines:
			break
			pass
		if 'PARSE_XML_' in lines:
			if value in lines:
				tmpfd.write(lines)
		elif flag == 2:
			if '};' in lines:
				flag = 0
				#tmpfd.write('=============\n')
				continue
			if ('add' in lines):
				oplist.append('add')
			elif ('mod' in lines):
				oplist.append('mod')
			elif ('del' in lines): 
				oplist.append('del')
			elif ('show' in lines):
				oplist.append('show')
		elif flag == 1:
			if '\"'+value+'\"' in lines:
				flag = 2
				#tmpfd.write('=============\n')
				continue
		elif 'struct module_node_parameter' in lines:
			flag = 1
		
	infd.close()
	tmpfd.close()
	return (oplist)

def gettypefromline(lines):
	"根据lines信息得到RESTful需要的类型"
	if 'PARSE_XML_STRING' in lines:
		tmp = 'String'
	elif 'PARSE_XML_INTERFACE' in lines:
		tmp = 'String'
	elif 'PARSE_XML_INT' in lines:
		tmp = 'Number'
	elif 'PARSE_XML_LONG' in lines:
		tmp = 'Number'
	elif 'PARSE_XML_ULONG' in lines:
                tmp = 'Number'
	elif 'PARSE_XML_CHAR' in lines:
		tmp = 'String'
	elif 'PARSE_XML_IP' in lines:
                tmp = 'String'
	elif 'PARSE_XML_STRUCT' in lines:
		tmp = 'Array'
	else:
		tmp = 'String'

	return (tmp)

def create_getfunc_from_show( outfd, value, example_cnt):
	"通过SHOW创建get方法"
	#构建Get方法头部
	outfd.write('/**\n')
	outfd.write(' * @api {GET} /v1.0/api/%s XXX 此处替换Get方法中文注释\n' %(value))
	outfd.write(' * @apiName XXX 此处替换为对Get方法的解释\n')
	outfd.write(' * @apiGroup XXX 此处替换为隶属的模块\n')
	outfd.write(' *\n *\n')

	#构建字段说明部分
	tmplist = []	
	tmpfd = open(tmpfile,'r')
	while True:
		lines = tmpfd.readline()
		if not lines:
			break
			pass
		if not ('PARSE_XML_' in lines):
			continue
		curtype = gettypefromline(lines)
		tmp = lines.split()
		#print('Get %s member: %s\n' %(curtype,tmp[1]))
		outfd.write(' * @apiSuccess {%s} %s XXX 此处替换为对%s的中文注释\n' %(curtype, tmp[1], tmp[1]))
		tmplist.append(tmp[1])
	tmpfd.close()
	outfd.write(' *\n')

	#构建响应部分
	outfd.write(' * @apiSuccessExample {json} Success-Response:\n')
	outfd.write(' *	HTTP/1.1 200 OK\n')
	outfd.write(' *	{\n')
	outfd.write(' *		\"data\": [\n')

	i=0
	while i<example_cnt:
		outfd.write(' *		{\n');
		for member in tmplist:
			outfd.write(' *			\"%s\": \"XXX\"\n' %(member))
		if i == 0:
			outfd.write(' *		},\n');
		else:
			outfd.write(' *		}\n');
		i = i + 1
	
	outfd.write(' *	],\n')
	outfd.write(' *	\"total": %d\n' %(example_cnt))
	outfd.write(' *	}\n')
	outfd.write(' */\n\n')
	return 0

def create_example_for_edit( outfd, paramlist ):
	"构建编辑命令的实例部分，默认情况为全参数传递"
	#传入参数实例
	outfd.write(' * @apiParamExample {json} Request-Example:\n')
	outfd.write(' *	{\n')
	for member in paramlist:
			outfd.write(' *		\"%s\": \"XXX\"\n' %(member))
	outfd.write(' *	}\n');
	outfd.write(' *\n')
	
	#成功执行响应实例
	outfd.write(' * @apiSuccessExample {json} Success-Response:\n')
	outfd.write(' *	HTTP/1.1 200 OK\n')
	outfd.write(' *	{\n')
	outfd.write(' *		\"code\":\"XXX\"\n')
	outfd.write(' *	}\n');
	outfd.write(' *\n')
	
	#失败执行响应实例
	outfd.write(' * @apiErrorExample {json} Error-Response:\n')
	outfd.write(' *	HTTP/1.1 422 Not Found\n')
	outfd.write(' *	{\n')
	outfd.write(' *		\"code\":\"XXX\"\n')
	outfd.write(' *		\"str\":\"XXX\"\n')
	outfd.write(' *	}\n');
	outfd.write(' *\n')
	return 0

def create_postfunc_from_add( outfd, value):
	"通过ADD构建post方法"
	#构建post头部
	outfd.write('/**\n')
	outfd.write(' * @api {POST} /v1.0/api/%s XXX 此处替换Post方法中文注释\n' %(value))
	outfd.write(' * @apiName XXX 此处替换为对Post方法的解释\n')
	outfd.write(' * @apiGroup XXX 此处替换为隶属的模块\n')
	outfd.write(' *\n *\n')
	
	#构建字段说明部分	
	tmplist = []
	tmpfd = open(tmpfile,'r')
	while True:
		lines = tmpfd.readline()
		if not lines:
			break
			pass
		if not ('PARSE_XML_' in lines):
			continue
		curtype = gettypefromline(lines)
		tmp = lines.split()
		#print('Get %s member: %s\n' %(curtype, tmp[1]))
		outfd.write(' * @apiParam {%s} %s XXX 此处替换为对%s的中文注释\n' %(curtype, tmp[1], tmp[1]))
		tmplist.append(tmp[1])
	tmpfd.close()
	outfd.write(' *\n')
	
	#构建实例部分
	create_example_for_edit(outfd, tmplist) 
	outfd.write(' */\n\n')
	return 0
	
def create_putfunc_from_mod( outfd, value):
	"通过MOD构建put方法"
	#构建put头部
	outfd.write('/**\n')
	outfd.write(' * @api {PUT} /v1.0/api/%s XXX 此处替换Put方法中文注释\n' %(value))
	outfd.write(' * @apiName XXX 此处替换为对Put方法的解释\n')
	outfd.write(' * @apiGroup XXX 此处替换为隶属的模块\n')
	outfd.write(' *\n *\n')
	
	#构建字段说明部分	
	tmplist = []
	tmpfd = open(tmpfile,'r')
	while True:
		lines = tmpfd.readline()
		if not lines:
			break
			pass
		if not ('PARSE_XML_' in lines):
			continue
		curtype = gettypefromline(lines)
		tmp = lines.split()
		#print('Get %s member: %s\n' %(curtype, tmp[1]))
		outfd.write(' * @apiParam {%s} %s XXX 此处替换为对%s的中文注释\n' %(curtype, tmp[1], tmp[1]))
		tmplist.append(tmp[1])
	tmpfd.close()
	outfd.write(' *\n')
	
	#构建实例部分
	create_example_for_edit(outfd, tmplist) 
	outfd.write(' */\n\n')
	return 0
	
def create_deletefunc_from_del( outfd, value):
	"通过MOD构建put方法"
	#构建put头部
	outfd.write('/**\n')
	outfd.write(' * @api {DELETE} /v1.0/api/%s XXX 此处替换Delete方法中文注释\n' %(value))
	outfd.write(' * @apiName XXX 此处替换为对Put方法的解释\n')
	outfd.write(' * @apiGroup XXX 此处替换为隶属的模块\n')
	outfd.write(' *\n *\n')
	
	#构建字段说明部分	
	tmplist = []
	tmpfd = open(tmpfile,'r')
	while True:
		lines = tmpfd.readline()
		if not lines:
			break
			pass
		if not ('PARSE_XML_' in lines):
			continue
		curtype = gettypefromline(lines)
		tmp = lines.split()
		#print('Get %s member: %s\n' %(curtype, tmp[1]))
		outfd.write(' * @apiParam {%s} %s XXX 此处替换为对%s的中文注释\n' %(curtype, tmp[1], tmp[1]))
		tmplist.append(tmp[1])
	tmpfd.close()
	outfd.write(' *\n')
	
	#构建实例部分
	create_example_for_edit(outfd, tmplist) 
	outfd.write(' */\n\n')
	return 0
#=====================================================================

def create_restfulapi_from_phpandc(phpfile, cfile, findvalue):
	"分析PHP文件，根据C文件中信息，在PHP文件中创建标准RESTful格式"

	if findvalue == '':
		findvalue = getvaluefromfile(phpfile)
	#print("从PHP文件获取的对象名: %s \n " %(findvalue))
	print('处理对象：%s, PHP_file : %s, C_file : %s' %(findvalue, phpfile, cfile)),
	
	apilist = getxmlinfofromcfile(cfile, findvalue)

	#查找phpfile的插入位置
	phpfd = open(phpfile,'r+')
	flag = 0
	while True:
		lines = phpfd.readline()
		if not lines:
			print('\t<%s> Read PHPfile error!\n' %(findvalue))
			return 0
		if 'namespace' in lines:
			flag = 1
			continue
		if (flag == 1) and ( 'use' in lines ):
			flag = 2
			break

	if flag != 2:
		phpfd.close()
		print('\t<%s>Find error!'%(findvalue))
		return

	posit = phpfd.tell()
	old = phpfd.read()
	phpfd.seek(posit)

	#去除模块名中的xml_前缀
	if 'xml_' in findvalue[0:4]:
		modname = findvalue[4:len(findvalue)]
	else:
		modname = findvalue

	#开始构建RESTful API注释
	phpfd.write('\n')
	if 'show' in apilist:
		create_getfunc_from_show(phpfd, modname, 2)

	if 'add' in apilist:
		create_postfunc_from_add(phpfd, modname)

	if 'mod' in apilist:
		create_putfunc_from_mod(phpfd, modname)
		
	if 'del' in apilist:
		create_deletefunc_from_del(phpfd, modname)
		
	phpfd.write(old);
	phpfd.close()
	print('构建 RESTful API注释 完成!!!\n')
	return 0;

def getcfile_for_value(cdir, value):
	"从C目录下的找到value定义的文件"
	cfilelist = os.listdir(cdir)
	flag = 0
	for cfilename in cfilelist:
		if '.c' in cfilename:
			#print '\t查找：%s' %(cfilename)
			f = open(cdir + '/' + cfilename,'r')
			while True:
				lines = f.readline()
				if not lines:
					break
					pass
				if '\"' + value + '\"' in lines:
					#print("%s : %s" %(cfilename, lines))
					flag = 1
					break
			f.close()
		if flag == 1:
			return (cfilename)
	return ''
	
def getoplist_from_dir(phpdir, cdir, phplist, clist, valuelist):
	"从PHP目录下检查需要生成RESTful API的PHPfile,Cfile,value列表"
	phpfilelist = os.listdir(phpdir)
	for phpfilename in phpfilelist:
		#print phpfilename
		tmpvalue = getvaluefromfile(phpdir + '/' +phpfilename)
		if tmpvalue != '':
			#print '得到tmpvalue:%s' %(tmpvalue)
			tmpcfilename = getcfile_for_value(cdir, tmpvalue)
			if tmpcfilename != '':
				#print '添加：%s: %s: %s' %(phpfilename,tmpcfilename,tmpvalue)
				phplist.append(phpfilename)
				clist.append(tmpcfilename)
				valuelist.append(tmpvalue)
	return 0

####add new edit restful php file function
def dos_to_unix(fname):  
	with open(fname,'rb+') as fobj:  
		data = fobj.read()  
		data = data.replace('\r\n', '\n')  
		fobj.seek(0, 0)  
		fobj.truncate()  
		fobj.write(data)  

def editRestfulApiPhpFile( infile ):   #以rb+方式打开再写入某些情况下会在文件末尾复制倒数三行写到文件尾部，导致多出三行冗余代码，需要以读写的方式打开，然后以w方式写入规避，可能为python本身的bug导致,或者文件整体长度变化导致？？？
	#"从文件中得到需要分析的字串"
	print '正在修改文件' +  infile
	with open(infile,'rb+') as f :
		lines = f.readlines()
		number = 0
		for line in lines:
			#print lines[number]
			if	',\"' in line or 'Group' in line or '/v1.0' in line:
				if (line.find(',\"') != -1):
					print('we will replace ,\" to \"')
					newline = lines[number].replace(',\"', '\"')
					lines[number] = newline
					# if lines[number+1].find("]") == -1 and lines[number+1].find("}") == -1:
						# newNoSpaceLine = line.rstrip('a b')
						# if newNoSpaceLine.endswith (',\n'):
							# print('end with ,')
							# print newNoSpaceLine
						# else:
							# #print('not end with ,')
							# newline = newNoSpaceLine.replace('\n', ',\n')
							# lines[number] = newline
							# #print 'here' + lines[number]
				if (line.find('/v1.0') != -1):
					print ('we will replace /v1.0 to \' \'')
					newline = line.replace('/v1.0', ' ')
					lines[number] = newline
					#print lines[number]
			if '@apiSuccess' in line:
				newline = line.replace(',', ' ', 1)
				lines[number] = newline
			number = number+1
	with open(infile,'w') as f:
		print 'number is' + bytes(number)
		f.writelines(lines)
	return ''
	
def editRestfulApiSencondTime( infile):
	#"过滤"XXX": "XXX"事例中末尾不包含,的事例， 在相关行的末尾添加,"
	with open(infile,'rb+') as f :
		lines = f.readlines()
		number = 0
		for line in lines:
			if '\"' in line:
				if line.find(':') != -1 and (line.find('[') == -1 and line.find('{') == -1):
					if lines[number+1].find("]") == -1 and lines[number+1].find("}") == -1:
						#print 'here is the line need , end'
						#print line
						newNoSpaceLine = line.rstrip()
						if newNoSpaceLine.endswith (','):
							print('second end with ,')
							print newNoSpaceLine
						else:
							print('second not end with ,')
							print newNoSpaceLine
							newline = newNoSpaceLine + ',\n'
							lines[number] = newline
							#print 'here' + lines[number]
				else:
					pass
					#print 'next line'
			number = number + 1
	
	with open(infile,'w') as f:
			print 'number is' + bytes(number)
			f.writelines(lines)
	print '完成文件' +  infile + '的修改'
	return ''
	
	
def getphpfileList_from_dir(phpdir,  phplist):
	#"从PHP目录下检查需要生成RESTful API的PHPfile,Cfile,value列表"
	phpfilelist = os.listdir(phpdir)
	for phpfilename in phpfilelist:
		#print phpfilename
				#print '添加：%s: %s: %s' %(phpfilename,tmpcfilename,tmpvalue)
		phplist.append(phpfilename)
	return 0

####add new edit restful php file function

#opts,args = getopt.getopt(sys.argv[2:], "ht:p:c:",["help","type=","php=","c="]) #optype = 0 #tphpdir = '' #tcdir = ''
#for op,value in opts:
#	if op in ("-h","--help"):
#		usage()
#	if op in ("-p","--php"):
#		tphpdir = value
#	if op in ("-c","--c"):
#		tcdir = value
#	if op in ("-t","--type"):
#		if value == 'file':
#			optype = 1
#		elif value == 'dir':
#			optype = 2
#
#if (optype != 1) and (optype != 2 ):
#	sys.exit()
#
#print '====================================================================='
#if optype == 1:
#	create_restfulapi_from_phpandc(tphpdir,tcdir,'')
#elif optype == 2:
#	tphplist = []
#	tclist = []
#	tvaluelist = []
#
#	getoplist_from_dir(tphpdir, tcdir, tphplist, tclist, tvaluelist)
#	#print tphplist,tclist,tvaluelist
#	i=0
#	while i < len(tphplist):
#		create_restfulapi_from_phpandc(tphpdir+'/'+tphplist[i], tcdir+'/'+tclist[i], tvaluelist[i])
#		i = i + 1
#print '====================================================================='
#
phplist = []
reload(sys)
sys.setdefaultencoding('utf-8')
getphpfileList_from_dir(sys.argv[1], phplist)
print phplist
for phpfile in phplist:
	if phpfile.endswith('.php'):
		print phpfile
		dos_to_unix(sys.argv[1] + '/' + phpfile)
		editRestfulApiPhpFile(sys.argv[1] + '/' + phpfile)
		editRestfulApiSencondTime(sys.argv[1] + '/' + phpfile)
		
#shutil.copyfile('./DnsZoneController.php.bak', './DnsZoneController.php')



