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

#include <QString>
#include <QDomDocument>
#include <retroshare/rsfiles.h>

class QDomElement ;
class QWidget;

class RsCollectionFile
{
	public:
		static const QString ExtensionString ;

		RsCollectionFile() ;

		// create from list of files and directories
		RsCollectionFile(const std::vector<DirDetails>& file_entries) ;

		// Loads file from disk.
		bool load(QWidget *parent);
		bool load(const QString& filename, bool showError = true);

		// Save to disk
		bool save(QWidget *parent) const ;
		bool save(const QString& filename) const ;

		// Download the content.
		void downloadFiles() const ;

		qulonglong size();

		static bool isCollectionFile(const QString& filename);

	private:
		struct DLinfo
		{
			QString name ;
			qulonglong size ;
			QString path ;
			QString hash ;
			bool filename_has_wrong_characters ;
		};

		void recursAddElements(QDomDocument&,const DirDetails&,QDomElement&) const ;
		void recursCollectDLinfos(const QDomElement&,std::vector<DLinfo>& dlinfos,const QString& current_dir,bool bad_chars_in_parent) const ;

		QDomDocument _xml_doc ;
		QString _filename ;

		friend class RsCollectionDialog ;
};

