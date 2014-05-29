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

class RsCollectionFile : public QObject
{
	Q_OBJECT

public:

	RsCollectionFile(QObject *parent = 0) ;
		// create from list of files and directories
	RsCollectionFile(const std::vector<DirDetails>& file_entries, QObject *parent = 0) ;
	virtual ~RsCollectionFile() ;

	static const QString ExtensionString ;



		// Loads file from disk.
		bool load(QWidget *parent);
	bool load(const QString& fileName, bool showError = true);

		// Save to disk
		bool save(QWidget *parent) const ;
	bool save(const QString& fileName) const ;

	// Open new collection
	bool openNewColl(QWidget *parent);
	// Open existing collection
	bool openColl(const QString& fileName, bool readOnly = false, bool showError = true);

		// Download the content.
		void downloadFiles() const ;

		qulonglong size();

	static bool isCollectionFile(const QString& fileName);

private slots:
	void saveColl(std::vector<ColFileInfo> colFileInfos, const QString& fileName);

	private:

		void recursAddElements(QDomDocument&,const DirDetails&,QDomElement&) const ;
	void recursAddElements(QDomDocument&,const ColFileInfo&,QDomElement&) const;
	void recursCollectColFileInfos(const QDomElement&,std::vector<ColFileInfo>& colFileInfos,const QString& current_dir,bool bad_chars_in_parent) const ;
	// check that the file is a valid rscollection file, and not a lol bomb or some shit like this
	static bool checkFile(const QString &fileName, bool showError);

		QDomDocument _xml_doc ;
	QString _fileName ;
	bool _saved;

		friend class RsCollectionDialog ;
};

