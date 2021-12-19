#!/usr/bin/python3

# RetroShare JSON API generator
#
# Copyright (C) 2019  selankon <selankon@selankon.xyz>
# Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License as published by the
# Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License along
# with this program. If not, see <https://www.gnu.org/licenses/>
#
# SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
# SPDX-License-Identifier: AGPL-3.0-only


# Original idea and implementation by G10h4ck (jsonapi-generator.cpp)
# Initial python reimplementation by Sehraf
#
# This python 3 script has superseded the original C++/Qt implementation this
# and is now used at build time in without depending on Qt.

import os
import sys

import xml.etree.ElementTree as ET
from string import Template


class MethodParam:
	_type = ''
	_name = ''
	_defval = ''
	_in = False
	_out = False
	_isMultiCallback = False
	_isSingleCallback = False


class TemplateOwn(Template):
	delimiter = '$%'
	pattern = '''
	\$%(?:
		(?P<escaped>\$\%) |   					# Escape sequence of two delimiters
		(?P<named>[_a-z][_a-z0-9]*)%\$      |   	# delimiter and a Python identifier
		{(?P<braced>[_a-z][_a-z0-9]*)}   |   	# delimiter and a braced identifier
		(?P<invalid>)              				# Other ill-formed delimiter exprs
	)
	'''


def getText(e):
	return "".join(e.itertext())


def processFile(file):
	try:
		dom1 = ET.parse(file).getroot()
	except FileNotFoundError:
		print('Can\'t open:', file)

	headerFileInfo = dom1[0].findall('location')[0].attrib['file']
	headerRelPath = os.path.dirname(headerFileInfo).split('/')[-1] + '/' + os.path.basename(headerFileInfo)

	for sectDef in dom1.findall('.//memberdef'):
		if sectDef.attrib['kind'] != 'variable' or sectDef.find('.//jsonapi') == None:
			continue

		instanceName = sectDef.find('name').text
		typeName = sectDef.find('type/ref').text

		typeFilePath = sectDef.find('type/ref').attrib['refid']

		try:
			dom2 = ET.parse(doxPrefix + typeFilePath + '.xml').getroot()
		except FileNotFoundError:
			print('Can\'t open:', doxPrefix + typeFilePath + '.xml')

		for member in dom2.findall('.//member'):
			refid = member.attrib['refid']
			methodName = member.find('name').text

			requiresAuth = True

			defFilePath = refid.split('_')[0] + '.xml'
			defFile = defFilePath

			print('Looking for', typeName, methodName, 'into', typeFilePath)

			try:
				defDoc = ET.parse(doxPrefix + defFilePath).getroot()
			except FileNotFoundError:
				print('Can\'t open:', doxPrefix + defFilePath)

			memberdef = None
			for tmpMBD in defDoc.findall('.//memberdef'):
				tmpId = tmpMBD.attrib['id']
				tmpKind = tmpMBD.attrib['kind']
				tmpJsonApiTagList = tmpMBD.findall('.//jsonapi')

				if len(tmpJsonApiTagList) != 0 and tmpId == refid and tmpKind == 'function':
					tmpJsonApiTag = tmpJsonApiTagList[0]

					tmpAccessValue = None
					if 'access' in tmpJsonApiTag.attrib:
						tmpAccessValue = tmpJsonApiTag.attrib['access']

					requiresAuth = 'unauthenticated' != tmpAccessValue;

					if 'manualwrapper' != tmpAccessValue:
						memberdef = tmpMBD

					break

			if memberdef == None:
				continue

			apiPath = '/' + instanceName + '/' + methodName

			retvalType = getText(memberdef.find('type'))
			# Apparently some xml declarations include new lines ('\n') and/or multiple spaces
			# Strip them using python magic
			retvalType = ' '.join(retvalType.split())

			paramsMap = {}
			orderedParamNames = []

			hasInput = False
			hasOutput = False
			hasSingleCallback = False
			hasMultiCallback = False
			callbackName = ''
			callbackParams = ''

			for tmpPE in memberdef.findall('param'):
				mp = MethodParam()

				pName = getText(tmpPE.find('declname'))
				tmpDefval = tmpPE.find('defval')
				mp._defval = getText(tmpDefval) if tmpDefval != None else ''
				pType = getText(tmpPE.find('type'))

				if pType.startswith('const '): pType = pType[6:]
				if pType.startswith('std::function'):
					if pType.endswith('&'): pType = pType[:-1]
					if pName.startswith('multiCallback'):
						mp._isMultiCallback = True
						hasMultiCallback = True
					elif pName.startswith('callback'):
						mp._isSingleCallback = True
						hasSingleCallback = True
					callbackName = pName
					callbackParams = pType
				else:
					pType = pType.replace('&', '').replace(' ', '')
				
				# Apparently some xml declarations include new lines ('\n') and/or multiple spaces
				# Strip them using python magic
				pType = ' '.join(pType.split())
				mp._defval = ' '.join(mp._defval.split())

				mp._type = pType
				mp._name = pName

				paramsMap[pName] = mp
				orderedParamNames.append(pName)

			for tmpPN in memberdef.findall('.//parametername'):
				tmpParam = paramsMap[tmpPN.text]
				tmpD = tmpPN.attrib['direction'] if 'direction' in tmpPN.attrib else ''

				if 'in' in tmpD:
					tmpParam._in = True
					hasInput = True
				if 'out' in tmpD:
					tmpParam._out = True
					hasOutput = True

			# Params sanity check
			for pmKey in paramsMap:
				pm = paramsMap[pmKey]
				if not (pm._isMultiCallback or pm._isSingleCallback or pm._in or pm._out):
					print('ERROR', 'Parameter:', pm._name, 'of:', apiPath,
							  'declared in:', headerRelPath,
							  'miss doxygen parameter direction attribute!',
							  defFile)
					sys.exit()

			functionCall = '\t\t'
			if retvalType != 'void':
				functionCall += retvalType + ' retval = '
				hasOutput = True
			functionCall += instanceName + '->' + methodName + '('
			functionCall += ', '.join(orderedParamNames) + ');\n'

			print(instanceName, apiPath, retvalType, typeName, methodName)
			for pn in orderedParamNames:
				mp = paramsMap[pn]
				print('\t', mp._type, mp._name, mp._in, mp._out)

			inputParamsDeserialization = ''
			if hasInput:
				inputParamsDeserialization += '\t\t{\n' 
				inputParamsDeserialization += '\t\t\tRsGenericSerializer::SerializeContext& ctx(cReq);\n' 
				inputParamsDeserialization += '\t\t\tRsGenericSerializer::SerializeJob j(RsGenericSerializer::FROM_JSON);\n';

			outputParamsSerialization = ''
			if hasOutput:
				outputParamsSerialization += '\t\t{\n'
				outputParamsSerialization += '\t\t\tRsGenericSerializer::SerializeContext& ctx(cAns);\n'
				outputParamsSerialization += '\t\t\tRsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);\n';

			paramsDeclaration = ''
			for pn in orderedParamNames:
				mp = paramsMap[pn]
				paramsDeclaration += '\t\t' + mp._type + ' ' + mp._name
				if mp._defval != '':
					paramsDeclaration += ' = ' + mp._defval
				paramsDeclaration += ';\n'
				if mp._in:
					inputParamsDeserialization += '\t\t\tRS_SERIAL_PROCESS('
					inputParamsDeserialization += mp._name + ');\n'
				if mp._out:
					outputParamsSerialization += '\t\t\tRS_SERIAL_PROCESS('
					outputParamsSerialization += mp._name + ');\n'

			if hasInput: 
				inputParamsDeserialization += '\t\t}\n'
			if retvalType != 'void': 
				outputParamsSerialization += '\t\t\tRS_SERIAL_PROCESS(retval);\n'
			if hasOutput:
				outputParamsSerialization += '\t\t}\n'

			captureVars = ''

			sessionEarlyClose = ''
			if hasSingleCallback:
				sessionEarlyClose = 'session->close();'

			sessionDelayedClose = ''
			if hasMultiCallback:
				sessionDelayedClose = """
		RsThread::async( [=]()
		{
			std::this_thread::sleep_for(
				std::chrono::seconds(maxWait+120) );
			auto lService = weakService.lock();
			if(!lService || lService->is_down()) return;
			lService->schedule( [=]()
			{
				auto session = weakSession.lock();
				if(session && session->is_open())
					session->close();
			} );
		} );
"""
				captureVars = 'this'

			callbackParamsSerialization = ''

			if hasSingleCallback or hasMultiCallback or (callbackParams.find('(') + 2 < callbackParams.find(')')):
				cbs = ''

				callbackParams = callbackParams.split('(')[1]
				callbackParams = callbackParams.split(')')[0]

				cbs += '\t\t\tRsGenericSerializer::SerializeContext ctx;\n'

				for cbPar in callbackParams.split(','):
					isConst = cbPar.startswith('const ')
					pSep = ' '
					isRef = '&' in cbPar
					if isRef: pSep = '&'
					sepIndex = cbPar.rfind(pSep) + 1
					cpt = cbPar[0:sepIndex][6:]
					cpn = cbPar[sepIndex:]

					cbs += '\t\t\tRsTypeSerializer::serial_process('
					cbs += 'RsGenericSerializer::TO_JSON, ctx, '
					if isConst:
						cbs += 'const_cast<'
						cbs += cpt
						cbs += '>('
					cbs += cpn
					if isConst: cbs += ')'
					cbs += ', "'
					cbs += cpn
					cbs += '" );\n'

				callbackParamsSerialization += cbs

			substitutionsMap = dict()
			substitutionsMap['paramsDeclaration'] = paramsDeclaration
			substitutionsMap['inputParamsDeserialization'] = inputParamsDeserialization
			substitutionsMap['outputParamsSerialization'] = outputParamsSerialization
			substitutionsMap['instanceName'] = instanceName
			substitutionsMap['functionCall'] = functionCall
			substitutionsMap['apiPath'] = apiPath
			substitutionsMap['sessionEarlyClose'] = sessionEarlyClose
			substitutionsMap['sessionDelayedClose'] = sessionDelayedClose
			substitutionsMap['captureVars'] = captureVars
			substitutionsMap['callbackName'] = callbackName
			substitutionsMap['callbackParams'] = callbackParams
			substitutionsMap['callbackParamsSerialization'] = callbackParamsSerialization
			substitutionsMap['requiresAuth'] = 'true' if requiresAuth else 'false'

			# print(substitutionsMap)

			templFilePath = sourcePath
			if hasMultiCallback  or hasSingleCallback:
				templFilePath += '/async-method-wrapper-template.cpp.tmpl'
			else:
				templFilePath += '/method-wrapper-template.cpp.tmpl'

			templFile = open(templFilePath, 'r')
			wrapperDef = TemplateOwn(templFile.read())

			tmp = wrapperDef.substitute(substitutionsMap)
			wrappersDefFile.write(tmp)

			cppApiIncludesSet.add('#include "' + headerRelPath + '"\n')


if len(sys.argv) != 3:
	print('Usage:', sys.argv[0], 'SOURCE_PATH OUTPUT_PATH Got:', sys.argv[:])
	sys.exit(-1)

sourcePath = str(sys.argv[1])
outputPath = str(sys.argv[2])
doxPrefix = outputPath + '/xml/'

try:
	wrappersDefFile = open(outputPath + '/jsonapi-wrappers.inl', 'w')
except FileNotFoundError:
	print('Can\'t open:', outputPath + '/jsonapi-wrappers.inl')

try:
	cppApiIncludesFile = open(outputPath + '/jsonapi-includes.inl', 'w');
except FileNotFoundError:
	print('Can\'t open:', outputPath + '/jsonapi-includes.inl')

cppApiIncludesSet = set()

filesIterator = None
try:
	filesIterator = os.listdir(doxPrefix)
except FileNotFoundError:
	print("Doxygen xml output dir not found: ", doxPrefix)
	os.exit(-1)

for file in filesIterator:
	if file.endswith("8h.xml"):
		processFile(os.path.join(doxPrefix, file))


for incl in cppApiIncludesSet:
	cppApiIncludesFile.write(incl)
