/*******************************************************************************
 * RetroShare JSON API                                                         *
 *                                                                             *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtXml/QtXml>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <iterator>
#include <functional>
#include <QVariant>
#include <cerrno>

struct MethodParam
{
	MethodParam() :
	    in(false), out(false), isMultiCallback(false), isSingleCallback(false){}

	QString type;
	QString name;
	QString defval;
	bool in;
	bool out;
	bool isMultiCallback;
	bool isSingleCallback;
};

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		qDebug() << "Usage: jsonapi-generator SOURCE_PATH OUTPUT_PATH";
		return -EINVAL;
	}

	QString sourcePath(argv[1]);
	QString outputPath(argv[2]);
	QString doxPrefix(outputPath+"/xml/");

	QString wrappersDefFilePath(outputPath + "/jsonapi-wrappers.inl");
	QFile wrappersDefFile(wrappersDefFilePath);
	wrappersDefFile.remove();
	if(!wrappersDefFile.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text))
	{
		qDebug() << "Can't open: " << wrappersDefFilePath;
		return -errno;
	}

	QString cppApiIncludesFilePath(outputPath + "/jsonapi-includes.inl");
	QFile cppApiIncludesFile(cppApiIncludesFilePath);
	cppApiIncludesFile.remove();
	if(!cppApiIncludesFile.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text))
	{
		qDebug() << "Can't open: " << cppApiIncludesFilePath;
		return -errno;
	}
	QSet<QString> cppApiIncludesSet;

	auto fatalError = [&](
	        std::initializer_list<QVariant> errors, int ernum = -EINVAL )
	{
		QString errorMsg;
		for(const QVariant& error: errors)
			errorMsg += error.toString() + " ";
		errorMsg.chop(1);
		QByteArray cppError(QString("#error "+errorMsg+"\n").toLocal8Bit());
		wrappersDefFile.write(cppError);
		cppApiIncludesFile.write(cppError);
		qDebug() << errorMsg;
		return ernum;
	};

	QDirIterator it(doxPrefix, QStringList() << "*8h.xml", QDir::Files);
	while(it.hasNext())
	{
		QDomDocument hDoc;
		QString hFilePath(it.next());
		QString parseError; int line, column;
		QFile hFile(hFilePath);
		if (!hFile.open(QIODevice::ReadOnly) ||
		        !hDoc.setContent(&hFile, &parseError, &line, &column))
		{
			qWarning() << "Error parsing:" << hFilePath
			           << parseError << line << column;
			continue;
		}

		QFileInfo headerFileInfo(hDoc.elementsByTagName("location").at(0)
		                         .attributes().namedItem("file").nodeValue());
		QString headerRelPath = headerFileInfo.dir().dirName() + "/" +
		        headerFileInfo.fileName();

		QDomNodeList sectiondefs = hDoc.elementsByTagName("memberdef");
		for(int j = 0; j < sectiondefs.size(); ++j)
		{
			QDomElement sectDef = sectiondefs.item(j).toElement();

			if( sectDef.attributes().namedItem("kind").nodeValue() != "variable"
			        || sectDef.elementsByTagName("jsonapi").isEmpty() )
				continue;

			QString instanceName =
			        sectDef.elementsByTagName("name").item(0).toElement().text();
			QString typeName =
			        sectDef.elementsByTagName("ref").item(0).toElement().text();
			QString typeFilePath(doxPrefix);
			typeFilePath += sectDef.elementsByTagName("ref").item(0)
			        .attributes().namedItem("refid").nodeValue();
			typeFilePath += ".xml";

			QDomDocument typeDoc;
			QFile typeFile(typeFilePath);
			if ( !typeFile.open(QIODevice::ReadOnly) ||
			     !typeDoc.setContent(&typeFile, &parseError, &line, &column) )
			{
				qWarning() << "Error parsing:" << typeFilePath
				           << parseError << line << column;
				continue;
			}

			QDomNodeList members = typeDoc.elementsByTagName("member");
			for (int i = 0; i < members.size(); ++i)
			{
				QDomNode member = members.item(i);
				QString refid(member.attributes().namedItem("refid").nodeValue());
				QString methodName(member.firstChildElement("name").toElement().text());
				bool requiresAuth = true;
				QString defFilePath(doxPrefix + refid.left(refid.lastIndexOf('_')) + ".xml");

				qDebug() << "Looking for" << typeName << methodName << "into"
				         << typeFilePath;

				QDomDocument defDoc;
				QFile defFile(defFilePath);
				if ( !defFile.open(QIODevice::ReadOnly) ||
				     !defDoc.setContent(&defFile, &parseError, &line, &column) )
				{
					qWarning() << "Error parsing:" << defFilePath
					           << parseError << line << column;
					continue;
				}

				QDomElement memberdef;
				QDomNodeList memberdefs = defDoc.elementsByTagName("memberdef");
				for (int k = 0; k < memberdefs.size(); ++k)
				{
					QDomElement tmpMBD = memberdefs.item(k).toElement();
					QString tmpId = tmpMBD.attributes().namedItem("id").nodeValue();
					QString tmpKind = tmpMBD.attributes().namedItem("kind").nodeValue();
					QDomNodeList tmpJsonApiTagList(tmpMBD.elementsByTagName("jsonapi"));

					if( tmpJsonApiTagList.count() && tmpId == refid &&
					        tmpKind == "function")
					{
						QDomElement tmpJsonApiTag = tmpJsonApiTagList.item(0).toElement();
						QString tmpAccessValue;
						if(tmpJsonApiTag.hasAttribute("access"))
							tmpAccessValue = tmpJsonApiTag.attributeNode("access").nodeValue();

						requiresAuth = "unauthenticated" != tmpAccessValue;

						if("manualwrapper" != tmpAccessValue)
							memberdef = tmpMBD;

						break;
					}
				}

				if(memberdef.isNull()) continue;

				QString apiPath("/" + instanceName + "/" + methodName);
				QString retvalType = memberdef.firstChildElement("type").text();
				QMap<QString,MethodParam> paramsMap;
				QStringList orderedParamNames;
				bool hasInput = false;
				bool hasOutput = false;
				bool hasSingleCallback = false;
				bool hasMultiCallback = false;
				QString callbackName;
				QString callbackParams;

				QDomNodeList params = memberdef.elementsByTagName("param");
				for (int k = 0; k < params.size(); ++k)
				{
					QDomElement tmpPE = params.item(k).toElement();
					MethodParam tmpParam;
					QString& pName(tmpParam.name);
					QString& pType(tmpParam.type);
					pName = tmpPE.firstChildElement("declname").text();
					QDomElement tmpDefval = tmpPE.firstChildElement("defval");
					if(!tmpDefval.isNull()) tmpParam.defval = tmpDefval.text();
					QDomElement tmpType = tmpPE.firstChildElement("type");
					pType = tmpType.text();
					if(pType.startsWith("const ")) pType.remove(0,6);
					if(pType.startsWith("std::function"))
					{
						if(pType.endsWith('&')) pType.chop(1);
						if(pName.startsWith("multiCallback"))
						{
							tmpParam.isMultiCallback = true;
							hasMultiCallback = true;
						}
						else if(pName.startsWith("callback"))
						{
							tmpParam.isSingleCallback = true;
							hasSingleCallback = true;
						}
						callbackName = pName;
						callbackParams = pType;
					}
					else
					{
						pType.replace(QString("&"), QString());
						pType.replace(QString(" "), QString());
					}

					paramsMap.insert(tmpParam.name, tmpParam);
					orderedParamNames.push_back(tmpParam.name);
				}

				QDomNodeList parameternames = memberdef.elementsByTagName("parametername");
				for (int k = 0; k < parameternames.size(); ++k)
				{
					QDomElement tmpPN = parameternames.item(k).toElement();
					MethodParam& tmpParam = paramsMap[tmpPN.text()];
					QString tmpD = tmpPN.attributes().namedItem("direction").nodeValue();
					if(tmpD.contains("in"))
					{
						tmpParam.in = true;
						hasInput = true;
					}
					if(tmpD.contains("out"))
					{
						tmpParam.out = true;
						hasOutput = true;
					}
				}

				// Params sanity check
				for( const MethodParam& pm : paramsMap)
					if( !(pm.isMultiCallback || pm.isSingleCallback
					      || pm.in || pm.out) )
						return fatalError(
						{ "Parameter:", pm.name, "of:", apiPath,
						  "declared in:", headerRelPath,
						  "miss doxygen parameter direction attribute!",
						  defFile.fileName() });

				QString functionCall("\t\t");
				if(retvalType != "void")
				{
					functionCall += retvalType + " retval = ";
					hasOutput = true;
				}
				functionCall += instanceName + "->" + methodName + "(";
				functionCall += orderedParamNames.join(", ") + ");\n";

				qDebug() << instanceName << apiPath << retvalType << typeName
				         << methodName;
				for (const QString& pn : orderedParamNames)
				{
					const MethodParam& mp(paramsMap[pn]);
					qDebug() << "\t" << mp.type << mp.name << mp.in << mp.out;
				}

				QString inputParamsDeserialization;
				if(hasInput)
				{
					inputParamsDeserialization +=
					        "\t\t{\n"
					        "\t\t\tRsGenericSerializer::SerializeContext& ctx(cReq);\n"
					        "\t\t\tRsGenericSerializer::SerializeJob j(RsGenericSerializer::FROM_JSON);\n";
				}

				QString outputParamsSerialization;
				if(hasOutput)
				{
					outputParamsSerialization +=
					        "\t\t{\n"
					        "\t\t\tRsGenericSerializer::SerializeContext& ctx(cAns);\n"
					        "\t\t\tRsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);\n";
				}

				QString paramsDeclaration;
				for (const QString& pn : orderedParamNames)
				{
					const MethodParam& mp(paramsMap[pn]);
					paramsDeclaration += "\t\t" + mp.type + " " + mp.name;
					if(!mp.defval.isEmpty())
						paramsDeclaration += " = " + mp.defval;
					paramsDeclaration += ";\n";
					if(mp.in)
						inputParamsDeserialization += "\t\t\tRS_SERIAL_PROCESS("
						        + mp.name + ");\n";
					if(mp.out)
						outputParamsSerialization += "\t\t\tRS_SERIAL_PROCESS("
						        + mp.name + ");\n";
				}

				if(hasInput) inputParamsDeserialization += "\t\t}\n";
				if(retvalType != "void")
					outputParamsSerialization +=
					        "\t\t\tRS_SERIAL_PROCESS(retval);\n";
				if(hasOutput) outputParamsSerialization += "\t\t}\n";

				QString captureVars;

				QString sessionEarlyClose;
				if(hasSingleCallback)
					sessionEarlyClose = "session->close();";

				QString sessionDelayedClose;
				if(hasMultiCallback)
				{
					sessionDelayedClose = "mService.schedule( [session](){session->close();}, std::chrono::seconds(maxWait+120) );";
					captureVars = "this";
				}

				QString callbackParamsSerialization;

				if(hasSingleCallback || hasMultiCallback ||
				        ((callbackParams.indexOf('(')+2) < callbackParams.indexOf(')')))
				{
					QString& cbs(callbackParamsSerialization);

					callbackParams = callbackParams.split('(')[1];
					callbackParams = callbackParams.split(')')[0];

					cbs += "\t\t\tRsGenericSerializer::SerializeContext ctx;\n";

					for (QString cbPar : callbackParams.split(','))
					{
						bool isConst(cbPar.startsWith("const "));
						QChar pSep(' ');
						bool isRef(cbPar.contains('&'));
						if(isRef) pSep = '&';
						int sepIndex = cbPar.lastIndexOf(pSep)+1;
						QString cpt(cbPar.mid(0, sepIndex));
						cpt.remove(0,6);
						QString cpn(cbPar.mid(sepIndex));

						cbs += "\t\t\tRsTypeSerializer::serial_process(";
						cbs += "RsGenericSerializer::TO_JSON, ctx, ";
						if(isConst)
						{
							cbs += "const_cast<";
							cbs += cpt;
							cbs += ">(";
						}
						cbs += cpn;
						if(isConst) cbs += ")";
						cbs += ", \"";
						cbs += cpn;
						cbs += "\" );\n";
					}
				}

				QMap<QString,QString> substitutionsMap;
				substitutionsMap.insert("paramsDeclaration", paramsDeclaration);
				substitutionsMap.insert("inputParamsDeserialization", inputParamsDeserialization);
				substitutionsMap.insert("outputParamsSerialization", outputParamsSerialization);
				substitutionsMap.insert("instanceName", instanceName);
				substitutionsMap.insert("functionCall", functionCall);
				substitutionsMap.insert("apiPath", apiPath);
				substitutionsMap.insert("sessionEarlyClose", sessionEarlyClose);
				substitutionsMap.insert("sessionDelayedClose", sessionDelayedClose);
				substitutionsMap.insert("captureVars", captureVars);
				substitutionsMap.insert("callbackName", callbackName);
				substitutionsMap.insert("callbackParams", callbackParams);
				substitutionsMap.insert("callbackParamsSerialization", callbackParamsSerialization);
				substitutionsMap.insert("requiresAuth", requiresAuth ? "true" : "false");

				QString templFilePath(sourcePath);
				if(hasMultiCallback || hasSingleCallback)
					templFilePath.append("/async-method-wrapper-template.cpp.tmpl");
				else templFilePath.append("/method-wrapper-template.cpp.tmpl");

				QFile templFile(templFilePath);
				templFile.open(QIODevice::ReadOnly);
				QString wrapperDef(templFile.readAll());

				QMap<QString,QString>::iterator it;
				for ( it = substitutionsMap.begin();
				      it != substitutionsMap.end(); ++it )
					wrapperDef.replace(QString("$%"+it.key()+"%$"), QString(it.value()), Qt::CaseSensitive);

				wrappersDefFile.write(wrapperDef.toLocal8Bit());

				cppApiIncludesSet.insert("#include \"" + headerRelPath + "\"\n");
			}
		}
	}

	for(const QString& incl : cppApiIncludesSet)
		cppApiIncludesFile.write(incl.toLocal8Bit());

	return 0;
}
