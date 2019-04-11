/*******************************************************************************
 * gui/common/RsCollection.h                                                   *
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

// Implements a RetroShare collection file. Such a file contains
//
// - a directory structure
// - retroshare links to put files in
//

#pragma once

#include <QObject>
#include <QString>
#include <QDomDocument>
#include <QFile>
#include <retroshare/rsfiles.h>
#include <QMetaType>

class QDomElement ;
class QWidget;

class ColFileInfo
{
	public:
	ColFileInfo(): name(""), size(0), path(""), hash(""), type(0), filename_has_wrong_characters(false), checked(false) {}

public:
	QString name ;
	qulonglong size ;
	QString path ;
	QString hash ;
	uint8_t type;
	bool filename_has_wrong_characters ;
	std::vector<ColFileInfo> children;
	bool checked;
};
Q_DECLARE_METATYPE(ColFileInfo)

class RsCollection : public QObject
{
	Q_OBJECT

public:

	RsCollection(QObject *parent = 0) ;
	// create from list of files and directories
	RsCollection(const std::vector<DirDetails>& file_entries, FileSearchFlags flags, QObject *parent = 0) ;
	RsCollection(const FileTree& fr);
	virtual ~RsCollection() ;

	void merge_in(const QString& fname,uint64_t size,const RsFileHash& hash) ;
	void merge_in(const FileTree& tree) ;

	static const QString ExtensionString ;

	// Loads file from disk.
	bool load(QWidget *parent);
	bool load(const QString& fileName, bool showError = true);

	// Save to disk
	bool save(QWidget *parent) const ;
	bool save(const QString& fileName) const ;

	// Open new collection
	bool openNewColl(QWidget *parent, QString fileName = "");
	// Open existing collection
	bool openColl(const QString& fileName, bool readOnly = false, bool showError = true);

	// Download the content.
	void downloadFiles() const ;
	// Auto Download all the content.
	void autoDownloadFiles() const ;

	qulonglong size();

	static bool isCollectionFile(const QString& fileName);

private slots:
	void saveColl(std::vector<ColFileInfo> colFileInfos, const QString& fileName);

private:

	void recursAddElements(QDomDocument&, const DirDetails&, QDomElement&, FileSearchFlags flags) const ;
	void recursAddElements(QDomDocument&,const ColFileInfo&,QDomElement&) const;
	void recursAddElements(QDomDocument& doc,const FileTree& ft,uint32_t index,QDomElement& e) const;

	void recursCollectColFileInfos(const QDomElement&,std::vector<ColFileInfo>& colFileInfos,const QString& current_dir,bool bad_chars_in_parent) const ;
	// check that the file is a valid rscollection file, and not a lol bomb or some shit like this
	static bool checkFile(const QString &fileName, bool showError);
	// Auto Download recursively.
	void autoDownloadFiles(ColFileInfo colFileInfo, QString dlDir) const ;

	QDomDocument _xml_doc ;
	QString _fileName ;
	bool _saved;
	QDomElement _root ;

	friend class RsCollectionDialog ;
};

