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

#include "ShareFilesDialog.h"

#include <QLocale>
//
ShareFilesDialog::ShareFilesDialog( QWidget * parent, Qt::WFlags f) 
	: QDialog(parent, f)
{
	setupUi(this);
	
	connect(shareOk,SIGNAL(clicked()), this, SLOT(FilenameShared()));
	connect(addfile_Btn,SIGNAL(clicked()), this, SLOT(addfileBrowse()));
	connect(DownloadList,SIGNAL(currentRowChanged(int)), this, SLOT(currIndex(int)));
	connect(remfile_Btn,SIGNAL(clicked()), this, SLOT(remove_File()));
	connect(DownloadList,SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(filenametoShare(QListWidgetItem *)));
}

void ShareFilesDialog::filenametoShare(QListWidgetItem *atname)
{
	filnameList = atname->text();
}

void ShareFilesDialog::FilenameShared()
{
	QDir dir;
	QString finm = dir.currentPath();
	QString cpypathto =dir.currentPath();
	
	finm.append("/DownLoad/");
	finm.append(filnameList);
	cpypathto.append("/SharedFolder/");
	cpypathto.append(filnameList);
	
	QFile share;
	share.copy(finm,cpypathto);
	messageBoxOk("Added!");
}
void ShareFilesDialog::addfileBrowse()
{
	QDir dir;
	int ind;
	QString pathseted =dir.currentPath();
	 
	pathseted.append("/DownLoad");
	
	QString fileshare = QFileDialog::getOpenFileName(this, tr("Open File..."),pathseted, tr("All Files (*)"));
	QString slash="/";
	
	ind=fileshare.lastIndexOf("/");
	filnameShared =fileshare.mid(ind+1,((fileshare.size())-ind));
	DownloadList->addItem(filnameShared);
	
}

void ShareFilesDialog::currIndex(int row)
{
	
	currrow=row;
	
}

void ShareFilesDialog::remove_File()
{
	
	QDir dir;
	QFile file;
	int ind;
	QString pathseted =dir.currentPath();
	QString path=dir.currentPath();
	path.append("/SharedFolder");
	QString fileshare = QFileDialog::getOpenFileName(this, tr("Open File..."),path, tr("All Files (*)"));
	ind=fileshare.lastIndexOf("/");
	filnameShared =fileshare.mid(ind+1,((fileshare.size())-ind));
	
	pathseted.append("/SharedFolder/");
	pathseted.append(filnameShared);
	QString queryWrn;
	queryWrn.clear();
	queryWrn.append("Do You Want to Delete ? ");
	queryWrn.append(pathseted);
	
	if ((QMessageBox::question(this, tr("Warning!"),queryWrn,QMessageBox::Ok|QMessageBox::No, QMessageBox::Ok))== QMessageBox::Ok)
	{
	file.remove(pathseted);
	}
	else
		return;
}

bool  ShareFilesDialog::messageBoxOk(QString msg)
 {
    QMessageBox mb("Share Manager MessageBox",msg,QMessageBox::Information,QMessageBox::Ok,0,0);
    mb.setButtonText( QMessageBox::Ok, "OK" );
    mb.exec();
    return true;
 }