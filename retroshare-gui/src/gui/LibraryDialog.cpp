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
#include "ShareManager.h"


#include "util/RsAction.h"
#include "msgs/ChanMsgDialog.h"
#include "library/FindWindow.h"

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

QString fileToFind;


/** Constructor */
LibraryDialog::LibraryDialog(QWidget *parent)
: MainPage(parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	ui.setupUi(this);
  
  	
  	PopulateList();
	
	connect(ui.organizerListView, SIGNAL(rightButtonClicked(QModelIndex,QPoint)), this, SLOT(ListLibrarymenu(QModelIndex,QPoint)));
	
	connect(ui.shareFiles_btn,SIGNAL(clicked()),this, SLOT(CallShareFilesBtn_library()));
	connect(ui.tileView_btn_library,SIGNAL(clicked()),this, SLOT(CallTileViewBtn_library()));
	connect(ui.showDetails_btn_library,SIGNAL(clicked()),this, SLOT(CallShowDetailsBtn_library()));
	connect(ui.createAlbum_btn_library,SIGNAL(clicked()),this, SLOT(CallCreateAlbumBtn_library()));
	connect(ui.deleteAlbum_btn_library,SIGNAL(clicked()),this, SLOT(CallDeleteAlbumBtn_library()));
	connect(ui.find_btn_library,SIGNAL(clicked()),this, SLOT(CallFindBtn_library()));

  	  /* Set header resize modes and initial section sizes  */
	QHeaderView * organizerheader = ui.organizertreeView->header();   
   
	organizerheader->resizeSection ( 0, 250 );
  

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}


void LibraryDialog::PopulateList()
{
	QDir DwnlFolder,ShrFolder,retroshareLib,treePath;

#if 0
	retroshareLib.mkdir("RetroShare Library");
	DwnlFolder.mkdir("RetroShare Library/Download");
	ShrFolder.mkdir("RetroShare Library/SharedFolder");
	LibShared=treePath.currentPath();
	LibShared.append("/RetroShare Library");
#else
	LibShared=Rshare::dataDirectory();
#endif

	QDirModel * dmodel=new QDirModel;
	ui.organizertreeView->setModel(dmodel);
	ui.organizertreeView->setRootIndex(dmodel->index(LibShared));
	ui.organizerListView->setModel(dmodel);
	ui.organizerListView->setViewMode(QListView::ListMode);
	ui.organizerListView->setWordWrap (true);

}


void LibraryDialog::ListLibrarymenu(QModelIndex index,QPoint pos) 
{
	ind=index;
	if(index.isValid())
	{
		bool indexselected=true;
		QMenu rmenu(this);
		rmenu.move(pos);
		if(indexselected)
		rmenu.addAction(QIcon(""),tr("Play"), this, SLOT(PlayFrmList()));
		if(indexselected)
		rmenu.addAction(QIcon(""),tr("Copy"), this, SLOT(copyFile()));
		if(indexselected)
		rmenu.addAction(QIcon(""),tr("Delete"), this, SLOT(DeleteFile()));
		if(indexselected)
		rmenu.addAction(QIcon(""),tr("Rename"), this, SLOT(RenameFile()));
		rmenu.exec();
	}
	else
		return;
}

void LibraryDialog::PlayFrmList() 
{
	QDirModel *dmodel=new QDirModel;
	QModelIndex parentIndex = dmodel->index(LibShared+"/Download");
    QModelIndex index = dmodel->index(ind.row(), 0, parentIndex);
    QString text = dmodel->data(index, Qt::DisplayRole).toString();
    
    filechose.clear();
    QDir dir;
    filechose.append(LibShared+"/Download/");
    filechose.append(text);
    if(filechose.contains("avi")|| filechose.contains("MP3")||filechose.contains("mp3")|| filechose.contains("wmv")||filechose.contains("wav")|| filechose.contains("dat")|| filechose.contains("mov")|| filechose.contains("mpeg")|| filechose.contains("mpg"))
    {
    	player();
	}
    else 
    	QMessageBox::information(this,"Information", "Not Supported By the Player");
}

void LibraryDialog::copyFile()
{
	
}

void LibraryDialog::DeleteFile()
{
	
}

void LibraryDialog::RenameFile()
{
	ui.organizerListView->openPersistentEditor (ind);
	progtime=new QTimer(this);
	progtime->start(100000);
	connect(progtime, SIGNAL(timeout()), this, SLOT(StopRename()));
	
}

void LibraryDialog::StopRename()
{
	ui.organizerListView->closePersistentEditor(ind);
	progtime->stop();
}


void LibraryDialog::CallShareFilesBtn_library()
{
	 ShareManager::showYourself();
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

QString LibraryDialog::filePass()
{
	fileToFind=ui.findEditmain->text();
	return fileToFind;
}

void LibraryDialog::CallFindBtn_library()
{
	filePass();
	FindWindow *files = new FindWindow(this);
	files->show();
}

void LibraryDialog:: player()
{
	QString avi;
	avi.append(filechose);
	QStringList argu;
	QString smlayer="./smplayer";
	argu<<avi;
	QProcess *retroshareplayer=new QProcess(this);
	retroshareplayer->start(smlayer,argu);
	
}

void LibraryDialog::browseFile()
 {
	QDir dir;
	 QString pathseted =dir.currentPath();
	 pathseted.append("/RetroShare Library/Download");
     filechose = QFileDialog::getOpenFileName(this, tr("Open File..."),
    		 	pathseted, tr("Media-Files (*.avi *.mp3 *.wmv *.wav *.dat *.mov *.mpeg);;All Files (*)"));
     //movieEdit->setText(filechose);
 }
