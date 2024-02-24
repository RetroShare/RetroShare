/*******************************************************************************
 * gui/common/RsCollection.cpp                                                 *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <stdexcept>
#include <retroshare/rsfiles.h>

#include "RsCollection.h"
#include "RsCollectionDialog.h"
#include "util/misc.h"

#include <QDir>
#include <QObject>
#include <QTextStream>
#include <QDomElement>
#include <QDomDocument>
#include <QMessageBox>
#include <QIcon>

// #define COLLECTION_DEBUG 1

const QString RsCollection::ExtensionString = QString("rscollection") ;

RsCollection::RsCollection(QObject *parent)
    : QObject(parent)
{
//	_root = _xml_doc.createElement("RsCollection");
//	_xml_doc.appendChild(_root);
}

RsCollection::RsCollection(const RsFileTree& ft)
    : mFileTree(ft)
{
}

RsCollection::RsCollection(const std::vector<DirDetails>& file_infos,FileSearchFlags flags, QObject *parent)
    : QObject(parent)
{
	if(! ( (flags & RS_FILE_HINTS_LOCAL) || (flags & RS_FILE_HINTS_REMOTE)))
	{
		std::cerr << "(EE) Wrong flags passed to RsCollection constructor. Please fix the code!" << std::endl;
		return ;
	}

	for(uint32_t i = 0;i<file_infos.size();++i)
        recursAddElements(mFileTree.root(),file_infos[i],flags) ;
}

RsCollection::~RsCollection()
{
}

void RsCollection::downloadFiles() const
{
#ifdef TODO
	// print out the element names of all elements that are direct children
	// of the outermost element.
	QDomElement docElem = _xml_doc.documentElement();

	std::vector<ColFileInfo> colFileInfos ;

	recursCollectColFileInfos(docElem,colFileInfos,QString(),false) ;

	RsCollectionDialog(_fileName, colFileInfos, false).exec() ;
#endif
}

void RsCollection::autoDownloadFiles() const
{
#ifdef TODO
	QDomElement docElem = _xml_doc.documentElement();

	std::vector<ColFileInfo> colFileInfos;

	recursCollectColFileInfos(docElem,colFileInfos,QString(),false);

	QString dlDir = QString::fromUtf8(rsFiles->getDownloadDirectory().c_str());

	foreach(ColFileInfo colFileInfo, colFileInfos)
	{
		autoDownloadFiles(colFileInfo, dlDir);
	}
#endif
}

void RsCollection::autoDownloadFiles(ColFileInfo colFileInfo, QString dlDir) const
{
	if (!colFileInfo.filename_has_wrong_characters)
	{
		QString cleanPath = dlDir + colFileInfo.path ;
		std::cout << "making directory " << cleanPath.toStdString() << std::endl;

		if(!QDir(QApplication::applicationDirPath()).mkpath(cleanPath))
			std::cerr << "Unable to make path: " + cleanPath.toStdString() << std::endl;

		if (colFileInfo.type==DIR_TYPE_FILE)
			rsFiles->FileRequest(colFileInfo.name.toUtf8().constData(),
			                     RsFileHash(colFileInfo.hash.toStdString()),
			                     colFileInfo.size,
			                     cleanPath.toUtf8().constData(),
			                     RS_FILE_REQ_ANONYMOUS_ROUTING,
			                     std::list<RsPeerId>());
	}
	foreach(ColFileInfo colFileInfoChild, colFileInfo.children)
	{
		autoDownloadFiles(colFileInfoChild, dlDir);
	}
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

void RsCollection::merge_in(const QString& fname,uint64_t size,const RsFileHash& hash)
{
    mFileTree.addFile(mFileTree.root(),fname.toStdString(),hash,size);
#ifdef TO_REMOVE
	ColFileInfo info ;
	info.type = DIR_TYPE_FILE ;
	info.name = fname ;
	info.size = size ;
	info.hash = QString::fromStdString(hash.toStdString()) ;

	recursAddElements(_xml_doc,info,_root) ;
#endif
}
void RsCollection::merge_in(const RsFileTree& tree)
{
    RsFileTree::DirData dd;
    tree.getDirectoryContent(tree.root(),dd);

    recursMergeTree(mFileTree.root(),tree,dd) ;
}

void RsCollection::recursMergeTree(RsFileTree::DirIndex parent,const RsFileTree& tree,const RsFileTree::DirData& dd)
{
    for(uint32_t i=0;i<dd.subfiles.size();++i)
    {
        RsFileTree::FileData fd;
        if(!tree.getFileContent(dd.subfiles[i],fd))
        {
            RsErr() << "Error while merging file trees. This should not happen. Report a bug!";
            return;
        }

        mFileTree.addFile(parent,fd.name,fd.hash,fd.size);
    }
    for(uint32_t i=0;i<dd.subdirs.size();++i)
    {
        RsFileTree::DirData ld;
        if(!tree.getDirectoryContent(dd.subdirs[i],ld))
        {
            RsErr() << "Error while merging file trees. This should not happen. Report a bug!";
            return;
        }

        auto new_dir_index = mFileTree.addDirectory(parent,ld.name);

        recursMergeTree(new_dir_index,tree,ld);
    }
}

#ifdef TO_REMOVE
void RsCollection::recursCollectColFileInfos(const QDomElement& e,std::vector<ColFileInfo>& colFileInfos,const QString& current_path, bool bad_chars_in_parent) const
{
	QDomNode n = e.firstChild() ;
#ifdef COLLECTION_DEBUG
	std::cerr << "Parsing element " << e.tagName().toStdString() << std::endl;
#endif

	while(!n.isNull()) 
	{
		QDomElement ee = n.toElement(); // try to convert the node to an element.

#ifdef COLLECTION_DEBUG
		std::cerr << "  Seeing child " << ee.tagName().toStdString() << std::endl;
#endif

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
#endif


void RsCollection::recursAddElements(RsFileTree::DirIndex parent, const DirDetails& dd, FileSearchFlags flags)
{
    if (dd.type == DIR_TYPE_FILE)
        mFileTree.addFile(parent,dd.name,dd.hash,dd.size);
    else if (dd.type == DIR_TYPE_DIR)
	{
        RsFileTree::DirIndex new_dir_index = mFileTree.addDirectory(parent,dd.name);

        for(uint32_t i=0;i<dd.children.size();++i)
		{
            if (!dd.children[i].ref)
				continue;

			DirDetails subDirDetails;

            if (!rsFiles->RequestDirDetails(dd.children[i].ref, subDirDetails, flags))
				continue;

            recursAddElements(new_dir_index,subDirDetails,flags) ;
		}
	}
}

#ifdef TO_REMOVE
void RsCollection::recursAddElements(QDomDocument& doc,const ColFileInfo& colFileInfo,QDomElement& e) const
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
			recursAddElements(doc,(*it),d) ;

		e.appendChild(d) ;
	}
}

void RsCollection::recursAddElements(
        QDomDocument& doc, const RsFileTree& ft, uint32_t index,
        QDomElement& e ) const
{
	std::vector<uint64_t> subdirs;
	std::vector<RsFileTree::FileData> subfiles ;
	std::string name;
	if(!ft.getDirectoryContent(name, subdirs, subfiles, index)) return;

	QDomElement d = doc.createElement("Directory") ;
	d.setAttribute(QString("name"),QString::fromUtf8(name.c_str())) ;
	e.appendChild(d) ;

	for (uint32_t i=0;i<subdirs.size();++i)
		recursAddElements(doc,ft,subdirs[i],d) ;

	for(uint32_t i=0;i<subfiles.size();++i)
	{
		QDomElement f = doc.createElement("File") ;

		f.setAttribute(QString("name"),QString::fromUtf8(subfiles[i].name.c_str())) ;
		f.setAttribute(QString("sha1"),QString::fromStdString(subfiles[i].hash.toStdString())) ;
		f.setAttribute(QString("size"),QString::number(subfiles[i].size)) ;

		d.appendChild(f) ;
	}
}
#endif

static void showErrorBox(const QString& fileName, const QString& error)
{
	QMessageBox mb(QMessageBox::Warning, QObject::tr("Failed to process collection file"), QObject::tr("The collection file %1 could not be opened.\nReported error is: \n\n%2").arg(fileName).arg(error), QMessageBox::Ok);
	mb.exec();
}

QString RsCollection::errorString(RsCollectionErrorCode code)
{
    switch(code)
    {
    default: [[fallthrough]] ;
    case RsCollectionErrorCode::UNKNOWN_ERROR:                 return tr("Unknown error");
    case RsCollectionErrorCode::NO_ERROR:                      return tr("No error");
    case RsCollectionErrorCode::FILE_READ_ERROR:               return tr("Error while openning file");
    case RsCollectionErrorCode::FILE_CONTAINS_HARMFUL_STRINGS: return tr("Collection file contains potentially harmful code");
    case RsCollectionErrorCode::INVALID_ROOT_NODE:             return tr("Invalid root node. RsCollection node was expected.");
    case RsCollectionErrorCode::XML_PARSING_ERROR:             return tr("XML parsing error in collection file");
    }
}

RsCollection::RsCollection(const QString& fileName, RsCollectionErrorCode& error)
{
    if (!checkFile(fileName,error))
        return ;

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        std::cerr << "Cannot open file " << fileName.toStdString() << " !!" << std::endl;
        error = RsCollectionErrorCode::FILE_READ_ERROR;
        //showErrorBox(fileName, QApplication::translate("RsCollectionFile", "Cannot open file %1").arg(fileName));
        return ;
    }

    QDomDocument xml_doc;
    bool ok = xml_doc.setContent(&file) ;

    if(!ok)
    {
        error = RsCollectionErrorCode::XML_PARSING_ERROR;
        return;
    }
    file.close();

    QDomNode root = xml_doc.elementsByTagName("RsCollection").at(0).toElement();

    if(root.isNull())
    {
        error = RsCollectionErrorCode::INVALID_ROOT_NODE;
        return;
    }

    recursParseXml(xml_doc,root,0);
    error = RsCollectionErrorCode::NO_ERROR;
}

// check that the file is a valid rscollection file, and not a lol bomb or some shit like this

bool RsCollection::checkFile(const QString& fileName, RsCollectionErrorCode& error)
{
    QFile file(fileName);
    error = RsCollectionErrorCode::NO_ERROR;

    if (!file.open(QIODevice::ReadOnly))
    {
        std::cerr << "Cannot open file " << fileName.toStdString() << " !!" << std::endl;
        error = RsCollectionErrorCode::FILE_READ_ERROR;

        //showErrorBox(fileName, QApplication::translate("RsCollectionFile", "Cannot open file %1").arg(fileName));
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

            if (n == max_size || file.atEnd())
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
                    //showErrorBox(file.fileName(), QApplication::translate("RsCollectionFile", "This file contains the string \"%1\" and is therefore an invalid collection file. \n\nIf you believe it is correct, remove the corresponding line from the file and re-open it with Retroshare.").arg(bad_strings[i].c_str()));
                    error = RsCollectionErrorCode::FILE_CONTAINS_HARMFUL_STRINGS;
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
    error = RsCollectionErrorCode::UNKNOWN_ERROR;
    return false;
}

#ifdef TO_REMOVE
bool RsCollection::load(QWidget *parent)
{
	QString fileName;
	if (!misc::getOpenFileName(parent, RshareSettings::LASTDIR_EXTRAFILE, QApplication::translate("RsCollectionFile", "Open collection file"), QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollection::ExtensionString + ")", fileName))
		return false;

	std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

	return load(fileName, true);
}
#endif

bool RsCollection::save(const QString& fileName) const
{
	QFile file(fileName);

	if (!file.open(QIODevice::WriteOnly))
	{
		std::cerr << "Cannot write to file " << fileName.toStdString() << " !!" << std::endl;
		return false;
	}

    QDomDocument xml_doc ;
    QDomElement root = xml_doc.createElement("RsCollection");

    RsFileTree::DirData root_data;
    if(!mFileTree.getDirectoryContent(mFileTree.root(),root_data))
        return false;

    if(!recursExportToXml(xml_doc,root,root_data))
        return false;

    xml_doc.appendChild(root);

    QTextStream stream(&file) ;
	stream.setCodec("UTF-8") ;

    stream << xml_doc.toString() ;
	file.close();

	return true;
}

bool RsCollection::recursParseXml(QDomDocument& doc,const QDomNode& e,const RsFileTree::DirIndex parent)
{
    QDomNode n = e.firstChild() ;
#ifdef COLLECTION_DEBUG
    std::cerr << "Parsing element " << e.tagName().toStdString() << std::endl;
#endif

    while(!n.isNull())
    {
        QDomElement ee = n.toElement(); // try to convert the node to an element.

#ifdef COLLECTION_DEBUG
        std::cerr << "  Seeing child " << ee.tagName().toStdString() << std::endl;
#endif

        if(ee.tagName() == QString("File"))
        {
            RsFileHash hash(ee.attribute(QString("sha1")).toStdString()) ;

            bool bad_chars_detected = false;
            std::string name = purifyFileName(ee.attribute(QString("name")), bad_chars_detected).toUtf8().constData() ;
            uint64_t size = ee.attribute(QString("size")).toULongLong() ;

            mFileTree.addFile(parent,name,hash,size);
#ifdef TO_REMOVE
            mFileTree.addFile(parent,)
            ColFileInfo newChild ;
            bool bad_chars_detected = false ;
            newChild.filename_has_wrong_characters = bad_chars_detected || bad_chars_in_parent ;
            newChild.size = ee.attribute(QString("size")).toULongLong() ;
            newChild.path = current_path ;
            newChild.type = DIR_TYPE_FILE ;

            colFileInfos.push_back(newChild) ;
#endif
        }
        else if(ee.tagName() == QString("Directory"))
        {
            bool bad_chars_detected = false ;
            std::string cleanDirName = purifyFileName(ee.attribute(QString("name")),bad_chars_detected).toUtf8().constData() ;

            RsFileTree::DirIndex new_dir_index = mFileTree.addDirectory(parent,cleanDirName);

            recursParseXml(doc,ee,new_dir_index);
#ifdef TO_REMOVE
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
#endif
        }

        n = n.nextSibling() ;
    }
}
bool RsCollection::recursExportToXml(QDomDocument& doc,QDomElement& e,const RsFileTree::DirData& dd) const
{
    for(uint32_t i=0;i<dd.subfiles.size();++i)
    {
        QDomElement f = doc.createElement("File") ;

        RsFileTree::FileData fd;
        if(!mFileTree.getFileContent(dd.subfiles[i],fd))
            return false;

        f.setAttribute(QString("name"),QString::fromUtf8(fd.name.c_str())) ;
        f.setAttribute(QString("sha1"),QString::fromStdString(fd.hash.toStdString())) ;
        f.setAttribute(QString("size"),QString::number(fd.size)) ;

        e.appendChild(f) ;
    }

    for(uint32_t i=0;i<dd.subdirs.size();++i)
    {
        RsFileTree::DirData di;
        if(!mFileTree.getDirectoryContent(dd.subdirs[i],di))
            return false;

        QDomElement d = doc.createElement("Directory") ;
        d.setAttribute(QString("name"),QString::fromUtf8(di.name.c_str())) ;

        if(!recursExportToXml(doc,d,di))
            return false;

        e.appendChild(d) ;
    }

    return true;
}

#ifdef TO_REMOVE
bool RsCollection::save(QWidget *parent) const
{
	QString fileName;
	if(!misc::getSaveFileName(parent, RshareSettings::LASTDIR_EXTRAFILE, QApplication::translate("RsCollectionFile", "Create collection file"), QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollection::ExtensionString + ")", fileName))
		return false;

	if (!fileName.endsWith("." + RsCollection::ExtensionString))
		fileName += "." + RsCollection::ExtensionString ;

	std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

	return save(fileName);
}


bool RsCollection::openNewColl(QWidget *parent, QString fileName)
{
	if(!misc::getSaveFileName(parent, RshareSettings::LASTDIR_EXTRAFILE
														, QApplication::translate("RsCollectionFile", "Create collection file")
														, QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollection::ExtensionString + ")"
														, fileName,0, QFileDialog::DontConfirmOverwrite))
		return false;

	if (!fileName.endsWith("." + RsCollection::ExtensionString))
		fileName += "." + RsCollection::ExtensionString ;

	std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

	QFile file(fileName) ;

	if(file.exists())
	{
		if (!checkFile(fileName,true)) return false;

		QMessageBox mb;
		mb.setText(tr("Save Collection File."));
		mb.setInformativeText(tr("File already exists.")+"\n"+tr("What do you want to do?"));
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

bool RsCollection::openColl(const QString& fileName, bool readOnly /* = false */, bool showError /* = true*/)
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
	}
	return false;
}
#endif

qulonglong RsCollection::size()
{
    return mFileTree.totalFileSize();

#ifdef TO_REMOVE
	QDomElement docElem = _xml_doc.documentElement();

	std::vector<ColFileInfo> colFileInfos;
	recursCollectColFileInfos(docElem, colFileInfos, QString(),false);

	uint64_t size = 0;

	for (uint32_t i = 0; i < colFileInfos.size(); ++i) {
		size += colFileInfos[i].size;
	}

	return size;
#endif
}

bool RsCollection::isCollectionFile(const QString &fileName)
{
	QString ext = QFileInfo(fileName).suffix().toLower();

	return (ext == RsCollection::ExtensionString);
}

#ifdef TO_REMOVE
void RsCollection::saveColl(std::vector<ColFileInfo> colFileInfos, const QString &fileName)
{

	QDomElement root = _xml_doc.elementsByTagName("RsCollection").at(0).toElement();
	while (root.childNodes().count()>0) root.removeChild(root.firstChild());
	for(uint32_t i = 0;i<colFileInfos.size();++i)
		recursAddElements(_xml_doc,colFileInfos[i],root) ;

	_saved=save(fileName);

}
#endif
