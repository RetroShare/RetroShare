/*
 * RetroShare JSON API
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCoreApplication>
#include <QDebug>
#include <QtXml>
#include <QDirIterator>
#include <QFileInfo>
#include <iterator>

struct MethodParam
{
	MethodParam() : in(false), out(false) {}

	QString type;
	QString name;
	bool in;
	bool out;
};

int main(int argc, char *argv[])
{
	if(argc != 3)
		qFatal("Usage: jsonapi-generator SOURCE_PATH OUTPUT_PATH");

	QString sourcePath(argv[1]);
	QString outputPath(argv[2]);
	QString doxPrefix(outputPath+"/xml/");

	QString wrappersDefFilePath(outputPath + "/jsonapi-wrappers.cpp");
	QFile wrappersDefFile(wrappersDefFilePath);
	wrappersDefFile.remove();
	if(!wrappersDefFile.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text))
		qFatal(QString("Can't open: " + wrappersDefFilePath).toLatin1().data());

	QString wrappersDeclFilePath(outputPath + "/jsonapi-wrappers.h");
	QFile wrappersDeclFile(wrappersDeclFilePath);
	wrappersDeclFile.remove();
	if(!wrappersDeclFile.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text))
		qFatal(QString("Can't open: " + wrappersDeclFilePath).toLatin1().data());

	QString wrappersRegisterFilePath(outputPath + "/jsonapi-register.inl");
	QFile wrappersRegisterFile(wrappersRegisterFilePath);
	wrappersRegisterFile.remove();
	if(!wrappersRegisterFile.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text))
		qFatal(QString("Can't open: " + wrappersRegisterFilePath).toLatin1().data());


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

		QFileInfo hfi(hFile);
		QString headerFileName(hfi.fileName());
		headerFileName.replace(QString("_8h.xml"), QString(".h"));

		QDomNodeList sectiondefs = hDoc.elementsByTagName("sectiondef");
		for(int j = 0; j < sectiondefs.size(); ++j)
		{
			QDomElement sectDef = sectiondefs.item(j).toElement();

			if( sectDef.attributes().namedItem("kind").nodeValue() != "var"
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
				QString wrapperName(instanceName+methodName+"Wrapper");

				QDomDocument defDoc;
				QString defFilePath(doxPrefix + refid.split('_')[0] + ".xml");
				QFile defFile(defFilePath);
				if ( !defFile.open(QIODevice::ReadOnly) ||
				     !defDoc.setContent(&defFile, &parseError, &line, &column) )
				{
					qWarning() << "Error parsing:" << defFilePath
					           << parseError << line << column;
					continue;
				}

				QDomElement memberdef;
				QDomNodeList memberdefs = typeDoc.elementsByTagName("memberdef");
				for (int k = 0; k < memberdefs.size(); ++k)
				{
					QDomElement tmpMBD = memberdefs.item(k).toElement();
					QString tmpId = tmpMBD.attributes().namedItem("id").nodeValue();
					QString tmpKind = tmpMBD.attributes().namedItem("kind").nodeValue();
					bool hasJsonApi = !tmpMBD.elementsByTagName("jsonapi").isEmpty();
					if( tmpId == refid && tmpKind == "function" && hasJsonApi )
					{
						memberdef = tmpMBD;
						break;
					}
				}

				if(memberdef.isNull()) continue;

				QString apiPath("/" + instanceName + "/" + methodName);
				QString retvalType = memberdef.firstChildElement("type").text();
				QMap<QString,MethodParam> paramsMap;
				QStringList orderedParamNames;

				QDomNodeList params = memberdef.elementsByTagName("param");
				for (int k = 0; k < params.size(); ++k)
				{
					QDomElement tmpPE = params.item(k).toElement();
					MethodParam tmpParam;
					tmpParam.name = tmpPE.firstChildElement("declname").text();
					QDomElement tmpType = tmpPE.firstChildElement("type");
					QString& pType(tmpParam.type);
					pType = tmpType.text();
					if(pType.startsWith("const ")) pType.remove(0,6);
					pType.replace(QString("&"), QString());
					pType.replace(QString(" "), QString());
					paramsMap.insert(tmpParam.name, tmpParam);
					orderedParamNames.push_back(tmpParam.name);
				}

				QDomNodeList parameternames = memberdef.elementsByTagName("parametername");
				for (int k = 0; k < parameternames.size(); ++k)
				{
					QDomElement tmpPN = parameternames.item(k).toElement();
					MethodParam& tmpParam = paramsMap[tmpPN.text()];
					QString tmpD = tmpPN.attributes().namedItem("direction").nodeValue();
					tmpParam.in = tmpD.contains("in");
					tmpParam.out = tmpD.contains("out");
				}

				qDebug() << instanceName << apiPath << retvalType << typeName
				         << methodName;
				for (const QString& pn : orderedParamNames)
				{
					const MethodParam& mp(paramsMap[pn]);
					qDebug() << "\t" << mp.type << mp.name << mp.in << mp.out;
				}

				QString retvalSerialization;
				if(retvalType != "void")
					retvalSerialization = "\t\t\tRS_SERIAL_PROCESS(retval);";

				QString paramsDeclaration;
				QString inputParamsDeserialization;
				QString outputParamsSerialization;
				for (const QString& pn : orderedParamNames)
				{
					const MethodParam& mp(paramsMap[pn]);
					paramsDeclaration += "\t\t" + mp.type + " " + mp.name + ";\n";
					if(mp.in)
						inputParamsDeserialization += "\t\t\tRS_SERIAL_PROCESS("
						        + mp.name + ");\n";
					if(mp.out)
						outputParamsSerialization += "\t\t\tRS_SERIAL_PROCESS("
						        + mp.name + ");\n";
				}

				QMap<QString,QString> substitutionsMap;
				substitutionsMap.insert("instanceName", instanceName);
				substitutionsMap.insert("methodName", methodName);
				substitutionsMap.insert("paramsDeclaration", paramsDeclaration);
				substitutionsMap.insert("inputParamsDeserialization", inputParamsDeserialization);
				substitutionsMap.insert("outputParamsSerialization", outputParamsSerialization);
				substitutionsMap.insert("retvalSerialization", retvalSerialization);
				substitutionsMap.insert("retvalType", retvalType);
				substitutionsMap.insert("callParamsList", orderedParamNames.join(", "));
				substitutionsMap.insert("wrapperName", wrapperName);
				substitutionsMap.insert("headerFileName", headerFileName);

				QFile templFile(sourcePath + "/method-wrapper-template.cpp.tmpl");
				templFile.open(QIODevice::ReadOnly);
				QString wrapperDef(templFile.readAll());

				QMap<QString,QString>::iterator it;
				for ( it = substitutionsMap.begin();
				      it != substitutionsMap.end(); ++it )
					wrapperDef.replace(QString("$%"+it.key()+"%$"), QString(it.value()), Qt::CaseSensitive);

				wrappersDefFile.write(wrapperDef.toLocal8Bit());

				QString wrapperDecl("void " + instanceName + methodName + "Wrapper(const std::shared_ptr<rb::Session> session);\n");
				wrappersDeclFile.write(wrapperDecl.toLocal8Bit());


				QString wrapperReg("registerHandler(\""+apiPath+"\", "+wrapperName+");\n");
				wrappersRegisterFile.write(wrapperReg.toLocal8Bit());
			}
		}
	}

	return 0;
}
