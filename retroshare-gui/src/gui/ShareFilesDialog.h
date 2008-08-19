/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008, defnax
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

#ifndef SHAREDFILE_DIALOGIMPL_H
#define SHAREDFILE_DIALOGIMPL_H

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>

#include "ui_ShareFilesDialog.h"
//
class ShareFilesDialog: public QDialog, public Ui::ShareFilesDialog
{
Q_OBJECT

public:
	ShareFilesDialog( QWidget * parent , Qt::WFlags f  );
	int currrow;
	bool messageBoxOk(QString);
	QString filnameList;
	QString filnameShared;
	
private slots:

	void FilenameShared();
	void addfileBrowse();
	void remove_File();
	void currIndex(int);
	void filenametoShare(QListWidgetItem *);
};
#endif





