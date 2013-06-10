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
#include "RsCollectionDialog.h"
#include "util/misc.h"

#include <QFile>
#include <QDir>
#include <QObject>
#include <QTextStream>
#include <QDomElement>
#include <QDomDocument>
#include <QMessageBox>
#include <QIcon>

const QString RsCollectionFile::ExtensionString = QString("rscollection") ;

RsCollectionFile::RsCollectionFile()
	: _xml_doc("RsCollection")
{
}

void RsCollectionFile::downloadFiles() const
{
	// print out the element names of all elements that are direct children
	// of the outermost element.
	QDomElement docElem = _xml_doc.documentElement();

	std::vector<DLinfo> dlinfos ;

	recursCollectDLinfos(docElem,dlinfos,QString(),false) ;

	RsCollectionDialog(_filename, dlinfos).exec() ;
}

static QString purifyFileName(const QString& input,bool& bad)
{
	static const QString bad_chars = "/\\\"*:?<>|" ;
	bad = false ;
	QString output = input ;

	for(int i=0;i<output.length();++i)
		for(int j=0;j<bad_chars.length();++j)
			if(output[i] == bad_chars[j])
			{
				output[i] = '_' ;
				bad = true ;
			}

	return output ;
}

void RsCollectionFile::recursCollectDLinfos(const QDomElement& e,std::vector<DLinfo>& dlinfos,const QString& current_path, bool bad_chars_in_parent) const
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
			bool bad_chars_detected = false ;
			i.name = purifyFileName(ee.attribute(QString("name")), bad_chars_detected) ;
			i.filename_has_wrong_characters = bad_chars_detected || bad_chars_in_parent ;
			i.size = ee.attribute(QString("size")).toULongLong() ;
			i.path = current_path ;

			dlinfos.push_back(i) ;
		}
		else if(ee.tagName() == QString("Directory"))
		{
			bool bad_chars_detected = false ;
			QString cleanDirName = purifyFileName(ee.attribute(QString("name")),bad_chars_detected) ;

			recursCollectDLinfos(ee,dlinfos,current_path + "/" + cleanDirName, bad_chars_in_parent || bad_chars_detected) ;
		}

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
			FileSearchFlags flags = RS_FILE_HINTS_LOCAL;

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

static void showErrorBox(const QString& filename, const QString& error)
{
	QMessageBox mb(QMessageBox::Warning, QObject::tr("Failed to process collection file"), QObject::tr("The collection file %1 could not be opened.\nReported error is: %2").arg(filename).arg(error), QMessageBox::Ok);
	mb.setWindowIcon(QIcon(":/images/rstray3.png"));
	mb.exec();
}

bool RsCollectionFile::load(const QString& filename, bool showError /* = true*/)
{
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
	{
		std::cerr << "Cannot open file " << filename.toStdString() << " !!" << std::endl;
		if (showError) {
			showErrorBox(filename, QApplication::translate("RsCollectionFile", "Cannot open file %1").arg(filename));
		}
		return false;
	}

	bool ok = _xml_doc.setContent(&file) ;
	file.close();

	if (ok) {
		_filename = filename;
	} else {
		if (showError) {
			showErrorBox(filename, QApplication::translate("RsCollectionFile", "Error parsing xml file"));
		}
	}

	return ok;
}

bool RsCollectionFile::load(QWidget *parent)
{
	QString filename;
	if (!misc::getOpenFileName(parent, RshareSettings::LASTDIR_EXTRAFILE, QApplication::translate("RsCollectionFile", "Open collection file"), QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollectionFile::ExtensionString + ")", filename))
		return false;

	std::cerr << "Got file name: " << filename.toStdString() << std::endl;

	return load(filename, true);
}

bool RsCollectionFile::save(const QString& filename) const
{
	QFile file(filename);

	if (!file.open(QIODevice::WriteOnly))
	{
		std::cerr << "Cannot write to file " << filename.toStdString() << " !!" << std::endl;
		return false;
	}

	QTextStream stream(&file) ;
	stream.setCodec("UTF-8") ;

	stream << _xml_doc.toString() ;

	file.close();

	return true;
}

bool RsCollectionFile::save(QWidget *parent) const
{
	QString filename;
	if(!misc::getSaveFileName(parent, RshareSettings::LASTDIR_EXTRAFILE, QApplication::translate("RsCollectionFile", "Create collection file"), QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollectionFile::ExtensionString + ")", filename))
		return false;

	if (!filename.endsWith("." + RsCollectionFile::ExtensionString))
		filename += "." + RsCollectionFile::ExtensionString ;

	std::cerr << "Got file name: " << filename.toStdString() << std::endl;

	return save(filename);
}

qulonglong RsCollectionFile::size()
{
	QDomElement docElem = _xml_doc.documentElement();

	std::vector<DLinfo> dlinfos;
	recursCollectDLinfos(docElem, dlinfos, QString(),false);

	uint64_t size = 0;

	for (uint32_t i = 0; i < dlinfos.size(); ++i) {
		size += dlinfos[i].size;
	}

	return size;
}

bool RsCollectionFile::isCollectionFile(const QString &filename)
{
	QString ext = QFileInfo(filename).suffix().toLower();

	return (ext == RsCollectionFile::ExtensionString);
}
