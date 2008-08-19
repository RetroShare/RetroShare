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


#include "rshare.h"
#include "LibraryDialog.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"
#include "rsiface/RemoteDirModel.h"
#include "ShareFilesDialog.h"

#include "util/RsAction.h"
#include "msgs/ChanMsgDialog.h"

#include <iostream>
#include <sstream>

#include <QtGui>
#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>
#include <QTimer>
#include <QMovie>
#include <QLabel>

/* Images for context menu icons */
#define IMAGE_DOWNLOAD       ":/images/download16.png"
#define IMAGE_PLAY           ":/images/start.png"
#define IMAGE_HASH_BUSY      ":/images/settings.png"
#define IMAGE_HASH_DONE      ":/images/friendsfolder24.png"
#define IMAGE_MSG            ":/images/message-mail.png"
#define IMAGE_ATTACHMENT     ":/images/attachment.png"
#define IMAGE_FRIEND         ":/images/peers_16x16.png"
#define IMAGE_PROGRESS       ":/images/browse-looking.gif"
#define IMAGE_LIBRARY        ":/images/library.png"


/** Constructor */
LibraryDialog::LibraryDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  	
  	PopulateList();
  	
  	QDirModel * dmodel=new QDirModel(this);
	ui.organizertreeView->setModel(dmodel);
	ui.organizerListView->setModel(dmodel);
	ui.organizerListView->setSpacing(10);
	QDir DwnlFolder,ShrFolder;
	
	
	connect(ui.shareFiles_btn,SIGNAL(clicked()),this, SLOT(CallShareFilesBtn_library()));
	connect(ui.tileView_btn_library,SIGNAL(clicked()),this, SLOT(CallTileViewBtn_library()));
	connect(ui.showDetails_btn_library,SIGNAL(clicked()),this, SLOT(CallShowDetailsBtn_library()));
	connect(ui.createAlbum_btn_library,SIGNAL(clicked()),this, SLOT(CallCreateAlbumBtn_library()));
	connect(ui.deleteAlbum_btn_library,SIGNAL(clicked()),this, SLOT(CallDeleteAlbumBtn_library()));
	connect(ui.find_btn_library,SIGNAL(clicked()),this, SLOT(CallFindBtn_library()));

  
  

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void LibraryDialog::PopulateList()
{
	QDirModel *dirmodel=new QDirModel(this);
	ui.organizertreeView->setModel(dirmodel);
	QModelIndex cmodel=dirmodel->index(QDir::rootPath());
	ui.organizertreeView->setRootIndex(cmodel); 
}


void LibraryDialog::CallShareFilesBtn_library()
{
	ShareFilesDialog *sf=new ShareFilesDialog(this,0);
	sf->show();
}

void LibraryDialog::CallTileViewBtn_library()
{
		//QMessageBox::information(this, tr("RetroShare"),tr("Will be Introducing this .. soon- tilesView in Library"));
}

void LibraryDialog::CallShowDetailsBtn_library()
{
		//QMessageBox::information(this, tr("RetroShare"),tr("Will be Introducing this .. soon- showdetails in Library"));
}

void LibraryDialog::CallCreateAlbumBtn_library()
{
	//QMessageBox::information(this, tr("RetroShare"),tr("Will be Introducing this .. soon- Create Album in Library"));
}

void LibraryDialog::CallDeleteAlbumBtn_library()
{
	//QMessageBox::information(this, tr("RetroShare"),tr("Will be Introducing this .. soon- Delete Album in Library"));
}


