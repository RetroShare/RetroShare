/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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

#ifndef TRANSFERPAGE_H
# define TRANSFERPAGE_H

# include <QWidget>

#include <retroshare-gui/configpage.h>
#include "ui_TransferPage.h"

class TransferPage: public ConfigPage
{
	Q_OBJECT

	public:
		TransferPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
		~TransferPage() {}

		/** Loads the settings for this page */
		virtual void load() ;

		virtual QPixmap iconPixmap() const { return QPixmap(":/icons/settings/filesharing.svg") ; }
        virtual QString pageName() const { return tr("Files") ; }
		virtual QString helpText() const { return ""; }

	public slots:
		void updateQueueSize(int) ;
		void updateDefaultStrategy(int) ;
		void updateDiskSizeLimit(int) ;
		void updateMaxTRUpRate(int);
		void updateEncryptionPolicy(int);
		void updateMaxUploadSlots(int);
		void updateFilePermDirectDL(int);
		void updateIgnoreLists();
		void updateMaxShareDepth(int);

		void editDirectories() ;
		void setIncomingDirectory();
		void setPartialsDirectory();
		void toggleAutoCheckDirectories(bool);

		void updateAutoCheckDirectories()       ;
		void updateAutoScanDirectoriesPeriod()  ;
		void updateShareDownloadDirectory()     ;
		void updateFollowSymLinks()             ;
		void updateIgnoreDuplicates()           ;

	private:

		Ui::TransferPage ui;
};

#endif // !TRANSFERPAGE_H

