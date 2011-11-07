/*************************************:***************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 - 2011 RetroShare Team
 *
 *  Cyril Soler (csoler@users.sourceforge.net)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <stdexcept>
#include <retroshare/rsfiles.h>

#include "RsCollectionFile.h"

#include <QFile>
#include <QDir>
#include <QObject>
#include <QMessageBox>
#include <QTextStream>
#include <QDomElement>
#include <QDomDocument>

const QString RsCollectionFile::ExtensionString = QString(".rscollection") ;

RsCollectionFile::RsCollectionFile(const QString& filename)
	: _xml_doc("RsCollection")
{
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
	{
		std::cerr << "Cannot open file " << filename.toStdString() << " !!" << std::endl;
		return;
	}

	bool ok = _xml_doc.setContent(&file) ;
	file.close();

	if(!ok)
		throw std::runtime_error("Error parsing xml file") ;
}


void RsCollectionFile::downloadFiles() const
{
	// print out the element names of all elements that are direct children
	// of the outermost element.
	QDomElement docElem = _xml_doc.documentElement();

	std::vector<DLinfo> dlinfos ;
	recursCollectDLinfos(docElem,dlinfos,QString()) ;

	QString msg(QObject::tr("About to download the following files:")+"<br><br>") ;

	for(uint32_t i=0;i<dlinfos.size();++i)
		msg += "   "+dlinfos[i].path + "/" + dlinfos[i].name + "<br>" ;

	QString dldir = QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()) ;

	if(QMessageBox::Ok == QMessageBox::critical(NULL,QObject::tr("About to download these files"),msg) )
	{
		std::cerr << "downloading all these files:" << std::endl;

		for(uint32_t i=0;i<dlinfos.size();++i)
		{
			std::cerr << dlinfos[i].name.toStdString() << " " << dlinfos[i].hash.toStdString() << " " << dlinfos[i].size << " " << dlinfos[i].path.toStdString() << std::endl;
			QString cleanPath = dldir + dlinfos[i].path ;
			std::cerr << "making directory " << cleanPath.toStdString() << std::endl;

			if(!QDir(cleanPath).mkpath(cleanPath))
				QMessageBox::warning(NULL,QObject::tr("Unable to make path"),QObject::tr("Unable to make path:")+"<br>  "+cleanPath) ;

			rsFiles->FileRequest(dlinfos[i].name.toUtf8().constData(), dlinfos[i].hash.toUtf8().constData(), dlinfos[i].size, cleanPath.toUtf8().constData(), RS_FILE_HINTS_NETWORK_WIDE, std::list<std::string>());
		}
	}
}

void RsCollectionFile::recursCollectDLinfos(const QDomElement& e,std::vector<DLinfo>& dlinfos,const QString& current_path) const
{
	QDomNode n = e.firstChild() ;

	std::cerr << "Parsing element " << e.tagName().toStdString() << std::endl;

	while(!n.isNull()) 
	{
		QDomElement ee = n.toElement(); // try to convert the node to an element.

		std::cerr << "  Seeing child " << ee.tagName().toStdString() << std::endl;

		if(ee.tagName() == QString("File"))
		{
			DLinfo i ;
			i.hash = ee.attribute(QString("sha1")) ;
			i.name = ee.attribute(QString("name")) ;
			i.size = ee.attribute(QString("size")).toULongLong() ;
			i.path = current_path ;

			dlinfos.push_back(i) ;
		}
		else if(ee.tagName() == QString("Directory"))
			recursCollectDLinfos(ee,dlinfos,current_path + "/" + ee.attribute(QString("name"))) ;

		n = n.nextSibling() ;
	}
}


void RsCollectionFile::recursAddElements(QDomDocument& doc,const DirDetails& details,QDomElement& e) const
{
	if (details.type == DIR_TYPE_FILE)
	{
		QDomElement f = doc.createElement("File") ;

		f.setAttribute(QString("name"),QString::fromUtf8(details.name.c_str())) ;
		f.setAttribute(QString("sha1"),QString::fromStdString(details.hash)) ;
		f.setAttribute(QString("size"),QString::number(details.count)) ;

		e.appendChild(f) ;
	}
	else if (details.type == DIR_TYPE_DIR)
	{
		QDomElement d = doc.createElement("Directory") ;

		d.setAttribute(QString("name"),QString::fromUtf8(details.name.c_str())) ;

		for (std::list<DirStub>::const_iterator it = details.children.begin(); it != details.children.end(); it++)
		{
			if (!it->ref) 
				continue;

			DirDetails subDirDetails;
			uint32_t flags = DIR_FLAGS_CHILDREN | DIR_FLAGS_LOCAL;

			if (!rsFiles->RequestDirDetails(it->ref, subDirDetails, flags)) 
				continue;

			recursAddElements(doc,subDirDetails,d) ;
		}

		e.appendChild(d) ;
	}
}

RsCollectionFile::RsCollectionFile(const std::vector<DirDetails>& file_infos) 
	: _xml_doc("RsCollection")
{
	QDomElement root = _xml_doc.createElement("RsCollection");
	_xml_doc.appendChild(root);

	for(uint32_t i = 0;i<file_infos.size();++i)
		recursAddElements(_xml_doc,file_infos[i],root) ;
}

void RsCollectionFile::save(const QString& filename) const
{
	QFile file(filename);

	if (!file.open(QIODevice::WriteOnly))
	{
		std::cerr << "Cannot write to file " << filename.toStdString() << " !!" << std::endl;
		return;
	}

	QTextStream stream(&file) ;

	stream << _xml_doc.toString() ;

	file.close();
}

