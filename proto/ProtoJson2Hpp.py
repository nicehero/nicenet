import os
import json
import logging
from logging import handlers

pre = '../'
class Logger(object):
    level_relations = {
        'debug':logging.DEBUG,
        'info':logging.INFO,
        'warning':logging.WARNING,
        'error':logging.ERROR,
        'crit':logging.CRITICAL
    }

    def __init__(self,filename,level='info',when='D',backCount=3,fmt='%(asctime)s - %(pathname)s[line:%(lineno)d] - %(levelname)s: %(message)s'):
        self.logger = logging.getLogger(filename)
        format_str = logging.Formatter(fmt)
        self.logger.setLevel(self.level_relations.get(level))
        sh = logging.StreamHandler()
        sh.setFormatter(format_str) 
        th = handlers.TimedRotatingFileHandler(filename=filename,when=when,backupCount=backCount,encoding='utf-8')
        th.setFormatter(format_str)
        self.logger.addHandler(sh)
        self.logger.addHandler(th)
log = Logger('ProtoJson2Hpp.log',level='debug')

#logging.basicConfig()
#logging.basicConfig(filename="ProtoJson2Hpp.log", filemode="a", format="%(asctime)s %(name)s:%(levelname)s:%(message)s", datefmt="%d-%M-%Y %H:%M:%S", level=logging.DEBUG)

structs = {}
cmdIDs = {}
cmdIDs2 = {}
files = {}
filesDeps = {}
structs['string'] = {}
structs['std::string'] = {}
structs['binary'] = {}
structs['OperUInt'] = {}
structs['StoreUInt'] = {}
tt1 = ['u','ui']
tt2 = ['8','16','32','64']
canUseIDs = range(101,60001)
idMap = {}

for i in tt1:
	for j in tt2:
		structs[i + j] = {}


def getVector(s):
	if s[0] == '<':
		return s[1:]
	return s

def build(filename):
	outFilename = filename.replace('.json','.hpp')
	print ('build: %s >> %s'%(filename,outFilename))
	sj = ""
	with open(filename,'rb') as f:
		sj = f.read()
	j = json.loads(sj)
	n = 1
	files[outFilename] = []
	for i in j:
		if not i.has_key("typeName"):
			log.logger.error(json.dumps(i))
			log.logger.critical('number :%d no typeName'%(n))
			return 2
		if not isinstance(i["typeName"],unicode):
			print (type(i["typeName"]))
			log.logger.error(json.dumps(i))
			log.logger.critical('number :%d typeName not str'%(n))
			return 2
		if structs.has_key(i["typeName"]):
			log.logger.error(json.dumps(i))
			log.logger.critical('duplicate typeName:%s'%(i["typeName"]))
			return 2
		if i["typeName"][-2:] == "ID":
			log.logger.error(json.dumps(i))
			log.logger.critical('typeName end can not be ID')
			return 2
		if not i.has_key("fields"):
			log.logger.error(json.dumps(i))
			log.logger.critical('number :%d no typeName'%(n))
			return 2
		if not isinstance(i["fields"],list):
			print (type(i["fields"]))
			log.logger.error(json.dumps(i))
			log.logger.critical('typeName :%s fields not list'%(i["typeName"]))
			return 2
		if (not i.has_key("isMsg")) or (not isinstance(i["isMsg"],bool)):
			i["isMsg"] = False
		if i["isMsg"] and (not cmdIDs2.has_key(i["typeName"] + "ID")) and (not cmdIDs.has_key(i["typeName"] + "ID")):
			cmdIDs[i["typeName"] + "ID"] = canUseIDs.pop(0)
			idMap[cmdIDs[i["typeName"] + "ID"]] = i["typeName"] + "ID"
		i["file"] = outFilename
		structs[i["typeName"]] = i
		files[outFilename].append(i)
		n += 1
	return 0
def build2(filename):
	outFilename = filename.replace('.json','.hpp')
	print ('build2: %s >> %s'%(filename,outFilename))
	filesDeps[outFilename] = {}
	for i in files[outFilename]:
		for j in i["fields"]:
			t = ""
			if (not j.has_key("name")) or (not isinstance(j["name"],unicode)):
				log.logger.error(json.dumps(i))
				log.logger.critical('field error name')
				return 2
			if (not j.has_key("type")) or (not isinstance(j["type"],unicode)):
				log.logger.error(json.dumps(i))
				log.logger.critical('field error type')
				return 2
			t = j["type"]
			j["vDepth"] = 0
			while 1:
				t2 = getVector(t)
				if t == t2:
					break;
				t = t2
				j["vDepth"] += 1
			j["t"] = t
			if not structs.has_key(t):
				log.logger.error(json.dumps(i))
				log.logger.critical('field type %s not found'%(t))
				return 2
			if t == i["typeName"]:
				log.logger.error(json.dumps(i))
				log.logger.critical('not support self ref')
				return 2
			if structs[t].has_key('file') and structs[t]['file'] != outFilename:
				filesDeps[outFilename][structs[t]['file']] = 1

def build3(filename):
	outFilename = filename.replace('.json','.hpp')
	print ('build3: %s >> %s'%(filename,outFilename))
	dh = '___' + filename.replace('.json','').upper() + '___'
	with open(outFilename,'wb') as f:
		f.write('#ifndef ' + dh + "\n")
		f.write('#define ' + dh + "\n")
		f.write('#include "%sMessage.h"\n' % (pre))
		for i in filesDeps[outFilename]:
			f.write('#include "' + i + '"\n')
		f.write('namespace Proto\n{\n')
		for i in files[outFilename]:
			if i['isMsg'] == True:
				f.write('\textern const ui16 %sID;\n'%(i["typeName"]))
			f.write('\tstruct ' + i["typeName"] + " : public nicehero::Serializable\n")
			f.write('\t{\n')
			for j in i['fields']:
				f.write('\t\t')
				for k in range(j['vDepth']):
					f.write('std::vector<')
				if j["t"] == 'string':
					j["t"] = 'std::string'
				if j["t"] == 'binary':
					j["t"] = 'nicehero::Binary'
				if j["t"] == 'OperUInt':
					j["t"] = 'nicehero::OperUInt'
				if j["t"] == 'StoreUInt':
					j["t"] = 'nicehero::StoreUInt'
				f.write(j["t"])
				for k in range(j['vDepth']):
					if (k != 0):
						f.write(' ')
					f.write('>')
				f.write(" " + j["name"])
				if j.has_key('default'):
					if j["t"] == 'std::string':
						f.write(' = "' + j['default'] + '"')
					else:
						f.write(' = ' + str(j['default']))
				f.write(";\n")
			f.write('''
\t\tui32 getSize() const;
\t\tvoid serializeTo(nicehero::Message& msg) const;
\t\tvoid unserializeFrom(nicehero::Message& msg);
''')
			if i['isMsg'] == True:
				f.write('\t\tui16 getID() const;\n')
			f.write('\t};\n\n')
		f.write('}\n\nnamespace Proto\n{\n')
		for i in files[outFilename]:
			f.write('\tinline nicehero::Message & operator << (nicehero::Message &m, const %s& p)\n\t{\n'%(i['typeName']))
			for j in i['fields']:
				if not (j.has_key("condition") and isinstance(j["condition"],unicode)):
					f.write('\t\tm << p.%s;\n'%(j["name"]))
				else:
					f.write('\t\tif (p.%s)\n'%(j["condition"]))
					f.write('\t\t\tm << p.%s;\n'%(j["name"]))
			f.write('\t\treturn m;\n\t}\n')
			
			f.write('\tinline nicehero::Message & operator >> (nicehero::Message &m, %s& p)\n\t{\n'%(i['typeName']))
			for j in i['fields']:
				if not (j.has_key("condition") and isinstance(j["condition"],unicode)):
					f.write('\t\tm >> p.%s;\n'%(j["name"]))
				else:
					f.write('\t\tif (p.%s)\n'%(j["condition"]))
					f.write('\t\t\tm >> p.%s;\n'%(j["name"]))
			f.write('\t\treturn m;\n\t}\n')
			f.write('''
\tinline void %s::serializeTo(nicehero::Message& msg) const
\t{
\t\tmsg << (*this);
\t}
\tinline void %s::unserializeFrom(nicehero::Message& msg)
\t{
\t\tmsg >> (*this);
\t}'''%(i['typeName'],i['typeName']))
			f.write('''
\tinline ui32 %s::getSize() const
\t{
\t\tui32 s = 0;
''' % (i['typeName']))
			for j in i['fields']:
				if not (j.has_key("condition") and isinstance(j["condition"],unicode)):
					f.write('\t\ts += nicehero::Serializable::getSize(%s);\n' % (j["name"]))
				else:
					f.write('\t\tif (%s)\n'%(j["condition"]))
					f.write('\t\t\ts += nicehero::Serializable::getSize(%s);\n' % (j["name"]))
			f.write('''\t\treturn s;
\t}
''')
			if i["isMsg"] == True:
				f.write('''
\tinline ui16 %s::getID() const
\t{
\t\treturn %sID;
\t}
'''%(i['typeName'],i['typeName']))
		f.write('}\n')
		f.write('#endif\n')
	return 0

filelist = os.listdir('./')

for i in filelist:
	if i == 'cmd_fix.json':
		with open('cmd_fix.json','rb') as f:
			cmdIDs = json.loads(f.read())
			if not isinstance(cmdIDs,dict):
				log.logger.critical('cmd_fix.json error')

for i in cmdIDs:
	if not isinstance(i,unicode):
		log.logger.critical('cmd_fix.json error:%s' % (str(i)))
	if not isinstance(cmdIDs[i],int):
		log.logger.critical('cmd_fix.json error2:%s' % (str(i)))
	if cmdIDs[i] < 0 or cmdIDs[i] > 60000:
		log.logger.critical('cmd_fix.json error3:%s' % (str(i)))
	if idMap.has_key(cmdIDs[i]):
		log.logger.critical('cmd_fix.json error4:%s duplicate' % (str(i)))
	idMap[cmdIDs[i]] = i
	
cmdIDs2 = cmdIDs
cmdIDs = {}
for i in filelist:
	if i == 'cmd.json':
		with open('cmd.json','rb') as f:
			cmdIDs = json.loads(f.read())
			if not isinstance(cmdIDs,dict):
				log.logger.critical('cmd.json error')
for i in list(cmdIDs.keys()):
	if not isinstance(i,unicode):
		log.logger.critical('cmd.json error:%s' % (str(i)))
	if not isinstance(cmdIDs[i],int):
		log.logger.critical('cmd.json error2:%s' % (str(i)))
	if cmdIDs2.has_key(i):
		del cmdIDs[i]
		continue
	if cmdIDs[i] < 0 or cmdIDs[i] > 60000:
		del cmdIDs[i]
		continue
	if idMap.has_key(cmdIDs[i]):
		del cmdIDs[i]
		continue
	idMap[cmdIDs[i]] = i
	
for i in idMap:
	try:
		canUseIDs.remove(i)
	except:
		pass



for i in filelist:
	if i == 'cmd.json':
		continue
	if i == 'cmd_fix.json':
		continue
	if i.find(".json") > 0:
		if build(i) > 0:
			exit(0)

for i in filelist:
	if i == 'cmd.json':
		continue
	if i == 'cmd_fix.json':
		continue
	if i.find(".json") > 0:
		if build2(i) > 0:
			exit(0)
	
for i in filelist:
	if i == 'cmd.json':
		continue
	if i == 'cmd_fix.json':
		continue
	if i.find(".json") > 0:
		if build3(i) > 0:
			exit(0)
	
#print idMap
print ('build cmd')
idV = []
for i in idMap:
	idV.append(i)
idV.sort()
with open('cmd.json','wb') as f:
	f.write('{\n')
	for i in idV:
		f.write('\t"' + idMap[i] + '" : ' + str(i))
		if i != idV[-1]:
			f.write(",\n")
		else:
			f.write("\n")
	f.write('}\n')

with open('cmd.cpp','wb') as f:
	f.write('''#include "%sType.h"
namespace Proto {
''' % (pre))
	for i in idV:
		f.write('\textern const ui16 ' + idMap[i] + ' = ' + str(i) + ';\n')
	f.write('}\n')







	