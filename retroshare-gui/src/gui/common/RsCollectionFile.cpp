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

#include <QDir>
#include <QObject>
#include <QTextStream>
#include <QDomElement>
#include <QDomDocument>
#include <QMessageBox>
#include <QIcon>

const QString RsCollectionFile::ExtensionString = QString("rscollection") ;

RsCollectionFile::RsCollectionFile(QObject *parent)
	: QObject(parent), _xml_doc("RsCollection")
{
}

RsCollectionFile::RsCollectionFile(const std::vector<DirDetails>& file_infos, QObject *parent)
	: QObject(parent), _xml_doc("RsCollection")
{
	QDomElement root = _xml_doc.createElement("RsCollection");
	_xml_doc.appendChild(root);

	for(uint32_t i = 0;i<file_infos.size();++i)
		recursAddElements(_xml_doc,file_infos[i],root) ;
}

RsCollectionFile::~RsCollectionFile()
{
}

void RsCollectionFile::downloadFiles() const
{
	// print out the element names of all elements that are direct children
	// of the outermost element.
	QDomElement docElem = _xml_doc.documentElement();

	std::vector<ColFileInfo> colFileInfos ;

	recursCollectColFileInfos(docElem,colFileInfos,QString(),false) ;

	RsCollectionDialog(_fileName, colFileInfos, false).exec() ;
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

void RsCollectionFile::recursCollectColFileInfos(const QDomElement& e,std::vector<ColFileInfo>& colFileInfos,const QString& current_path, bool bad_chars_in_parent) const
{
	QDomNode n = e.firstChild() ;

	std::cerr << "Parsing element " << e.tagName().toStdString() << std::endl;

	while(!n.isNull()) 
	{
		QDomElement ee = n.toElement(); // try to convert the node to an element.

		std::cerr << "  Seeing child " << ee.tagName().toStdString() << std::endl;

		if(ee.tagName() == QString("File"))
		{
			ColFileInfo newChild ;
			newChild.hash = ee.attribute(QString("sha1")) ;
			bool bad_chars_detected = false ;
			newChild.name = purifyFileName(ee.attribute(QString("name")), bad_chars_detected) ;
			newChild.filename_has_wrong_characters = bad_chars_detected || bad_chars_in_parent ;
			newChild.size = ee.attribute(QString("size")).toULongLong() ;
			newChild.path = current_path ;
			newChild.type = DIR_TYPE_FILE ;

			colFileInfos.push_back(newChild) ;
		}
		else if(ee.tagName() == QString("Directory"))
		{
			ColFileInfo newParent ;
			bool bad_chars_detected = false ;
			QString cleanDirName = purifyFileName(ee.attribute(QString("name")),bad_chars_detected) ;
			newParent.name=cleanDirName;
			newParent.filename_has_wrong_characters = bad_chars_detected || bad_chars_in_parent ;
			newParent.size = 0;
			newParent.path = current_path ;
			newParent.type = DIR_TYPE_DIR ;

			recursCollectColFileInfos(ee,newParent.children,current_path + "/" + cleanDirName, bad_chars_in_parent || bad_chars_detected) ;
			uint32_t size = newParent.children.size();
			for(uint32_t i=0;i<size;++i)
			{
				const ColFileInfo &colFileInfo = newParent.children[i];
				newParent.size +=colFileInfo.size ;
			}

			colFileInfos.push_back(newParent) ;
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
        f.setAttribute(QString("sha1"),QString::fromStdString(details.hash.toStdString())) ;
		f.setAttribute(QString("size"),QString::number(details.count)) ;

		e.appendChild(f) ;
	}
	else if (details.type == DIR_TYPE_DIR)
	{
		QDomElement d = doc.createElement("Directory") ;

		d.setAttribute(QString("name"),QString::fromUtf8(details.name.c_str())) ;

		for (std::list<DirStub>::const_iterator it = details.children.begin(); it != details.children.end(); ++it)
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

void RsCollectionFile::recursAddElements(QDomDocument& doc,const ColFileInfo& colFileInfo,QDomElement& e) const
{
	if (colFileInfo.type == DIR_TYPE_FILE)
	{
		QDomElement f = doc.createElement("File") ;

		f.setAttribute(QString("name"),colFileInfo.name) ;
		f.setAttribute(QString("sha1"),colFileInfo.hash) ;
		f.setAttribute(QString("size"),QString::number(colFileInfo.size)) ;

		e.appendChild(f) ;
}
	else if (colFileInfo.type == DIR_TYPE_DIR)
	{
		QDomElement d = doc.createElement("Directory") ;

		d.setAttribute(QString("name"),colFileInfo.name) ;

		for (std::vector<ColFileInfo>::const_iterator it = colFileInfo.children.begin(); it != colFileInfo.children.end(); ++it)
{
			recursAddElements(doc,(*it),d) ;
		}

		e.appendChild(d) ;
	}
}

static void showErrorBox(const QString& fileName, const QString& error)
{
	QMessageBox mb(QMessageBox::Warning, QObject::tr("Failed to process collection file"), QObject::tr("The collection file %1 could not be opened.\nReported error is: \n\n%2").arg(fileName).arg(error), QMessageBox::Ok);
	mb.exec();
}

bool RsCollectionFile::load(const QString& fileName, bool showError /* = true*/)
{

	if (!checkFile(fileName,showError)) return false;
	QFile file(fileName);

	if (!file.open(QIODevice::ReadOnly))
	{
		std::cerr << "Cannot open file " << fileName.toStdString() << " !!" << std::endl;
		if (showError) {
			showErrorBox(fileName, QApplication::translate("RsCollectionFile", "Cannot open file %1").arg(fileName));
		}
		return false;
	}

	bool ok = _xml_doc.setContent(&file) ;
	file.close();

	if (ok) {
		_fileName = fileName;
	} else {
		if (showError) {
			showErrorBox(fileName, QApplication::translate("RsCollectionFile", "Error parsing xml file"));
		}
	}

	return ok;
	}

	// check that the file is a valid rscollection file, and not a lol bomb or some shit like this
bool RsCollectionFile::checkFile(const QString& fileName, bool showError)
{
	QFile file(fileName);

	if (!file.open(QIODevice::ReadOnly))
	{
		std::cerr << "Cannot open file " << fileName.toStdString() << " !!" << std::endl;
		if (showError) {
			showErrorBox(fileName, QApplication::translate("RsCollectionFile", "Cannot open file %1").arg(fileName));
		}
		return false;
	}
	if (file.reset()){
	std::cerr << "Checking this file for bomb elements and various wrong stuff" << std::endl;
	char c ;

	std::vector<std::string> bad_strings ;
	bad_strings.push_back(std::string("<!entity ")) ;
	static const int max_size = 12 ; // should be as large as the largest element in bad_strings
	char current[max_size] = { 0,0,0,0,0,0,0,0,0,0,0,0 } ;
	int n=0 ;

		while( !file.atEnd() || n >= 0)
	{
			if (!file.atEnd())
				file.getChar(&c);
			else
				c=0;

		if(c == '\t' || c == '\n' || c == '\b' || c == '\r')
			continue ;

			if(n == max_size || file.atEnd())
			for(int i=0;i<n-1;++i)
				current[i] = current[i+1] ;

		if(n == max_size)
			--n ;

		if(c >= 'A' && c <= 'Z') c += 'a' - 'A' ;

			if(!file.atEnd())
			current[n] = c ;
		else
			current[n] = 0 ;

		//std::cerr << "n==" << n <<" Checking string " << std::string(current,n+1)  << " c = " << std::hex << (int)c << std::dec << std::endl;

			for(uint i=0;i<bad_strings.size();++i)
			if(std::string(current,bad_strings[i].length()) == bad_strings[i])
			{
					showErrorBox(file.fileName(), QApplication::translate("RsCollectionFile", "This file contains the string \"%1\" and is therefore an invalid collection file. \n\nIf you believe it is correct, remove the corresponding line from the file and re-open it with Retroshare.").arg(bad_strings[i].c_str()));
					file.close();
				return false ;
				//std::cerr << "Bad string detected" << std::endl;
			}

			if(file.atEnd())
			n-- ;
		else if(n < max_size)
			++n ;
	}
	file.close();
		return true;
		}
	return false;
}

bool RsCollectionFile::load(QWidget *parent)
{
	QString fileName;
	if (!misc::getOpenFileName(parent, RshareSettings::LASTDIR_EXTRAFILE, QApplication::translate("RsCollectionFile", "Open collection file"), QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollectionFile::ExtensionString + ")", fileName))
		return false;

	std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

	return load(fileName, true);
}

bool RsCollectionFile::save(const QString& fileName) const
{
	QFile file(fileName);

	if (!file.open(QIODevice::WriteOnly))
	{
		std::cerr << "Cannot write to file " << fileName.toStdString() << " !!" << std::endl;
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
	QString fileName;
	if(!misc::getSaveFileName(parent, RshareSettings::LASTDIR_EXTRAFILE, QApplication::translate("RsCollectionFile", "Create collection file"), QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollectionFile::ExtensionString + ")", fileName))
		return false;

	if (!fileName.endsWith("." + RsCollectionFile::ExtensionString))
		fileName += "." + RsCollectionFile::ExtensionString ;

	std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

	return save(fileName);
}


bool RsCollectionFile::openNewColl(QWidget *parent)
{
	QString fileName;
	if(!misc::getSaveFileName(parent, RshareSettings::LASTDIR_EXTRAFILE
														, QApplication::translate("RsCollectionFile", "Create collection file")
														, QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollectionFile::ExtensionString + ")"
														, fileName,0, QFileDialog::DontConfirmOverwrite))
		return false;

	if (!fileName.endsWith("." + RsCollectionFile::ExtensionString))
		fileName += "." + RsCollectionFile::ExtensionString ;

	std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

	QFile file(fileName) ;

	if(file.exists())
	{
		if (!checkFile(fileName,true)) return false;

		QMessageBox mb;
		mb.setText(tr("Save Collection File."));
		mb.setInformativeText(tr("File already exist.")+"\n"+tr("What do you want to do?"));
		QAbstractButton *btnOwerWrite = mb.addButton(tr("Overwrite"), QMessageBox::YesRole);
		QAbstractButton *btnMerge = mb.addButton(tr("Merge"), QMessageBox::NoRole);
		QAbstractButton *btnCancel = mb.addButton(tr("Cancel"), QMessageBox::ResetRole);
		mb.setIcon(QMessageBox::Question);
		mb.exec();

		if (mb.clickedButton()==btnOwerWrite) {
			//Nothing to do _xml_doc already up to date
		} else if (mb.clickedButton()==btnMerge) {
			//Open old file to merge it with _xml_doc
			QDomDocument qddOldFile("RsCollection");
			if (qddOldFile.setContent(&file)) {
				QDomElement docOldElem = qddOldFile.elementsByTagName("RsCollection").at(0).toElement();
				std::vector<ColFileInfo> colOldFileInfos;
				recursCollectColFileInfos(docOldElem,colOldFileInfos,QString(),false);

				QDomElement root = _xml_doc.elementsByTagName("RsCollection").at(0).toElement();
				for(uint32_t i = 0;i<colOldFileInfos.size();++i){
					recursAddElements(_xml_doc,colOldFileInfos[i],root) ;
				}
			}

		} else if (mb.clickedButton()==btnCancel) {
			return false;
		} else {
			return false;
		}

	}//if(file.exists())

	_fileName=fileName;
	std::vector<ColFileInfo> colFileInfos ;

	recursCollectColFileInfos(_xml_doc.documentElement(),colFileInfos,QString(),false) ;

	RsCollectionDialog* rcd = new RsCollectionDialog(fileName, colFileInfos,true);
	connect(rcd,SIGNAL(saveColl(std::vector<ColFileInfo>, QString)),this,SLOT(saveColl(std::vector<ColFileInfo>, QString))) ;
	_saved=false;
	rcd->exec() ;
	delete rcd;

	return _saved;
}

bool RsCollectionFile::openColl(const QString& fileName, bool readOnly /* = false */, bool showError /* = true*/)
{
	if (load(fileName, showError)) {
		std::vector<ColFileInfo> colFileInfos ;

		recursCollectColFileInfos(_xml_doc.documentElement(),colFileInfos,QString(),false) ;

		RsCollectionDialog* rcd = new RsCollectionDialog(fileName, colFileInfos, true, readOnly);
		connect(rcd,SIGNAL(saveColl(std::vector<ColFileInfo>, QString)),this,SLOT(saveColl(std::vector<ColFileInfo>, QString))) ;
		_saved=false;
		rcd->exec() ;
		delete rcd;

		return _saved;
	}//if (load(fileName, showError))
	return false;
}

qulonglong RsCollectionFile::size()
{
	QDomElement docElem = _xml_doc.documentElement();

	std::vector<ColFileInfo> colFileInfos;
	recursCollectColFileInfos(docElem, colFileInfos, QString(),false);

	uint64_t size = 0;

	for (uint32_t i = 0; i < colFileInfos.size(); ++i) {
		size += colFileInfos[i].size;
	}

	return size;
}

bool RsCollectionFile::isCollectionFile(const QString &fileName)
{
	QString ext = QFileInfo(fileName).suffix().toLower();

	return (ext == RsCollectionFile::ExtensionString);
}

void RsCollectionFile::saveColl(std::vector<ColFileInfo> colFileInfos, const QString &fileName)
{

	QDomElement root = _xml_doc.elementsByTagName("RsCollection").at(0).toElement();
	while (root.childNodes().count()>0) root.removeChild(root.firstChild());
	for(uint32_t i = 0;i<colFileInfos.size();++i)
		recursAddElements(_xml_doc,colFileInfos[i],root) ;

	_saved=save(fileName);

}
