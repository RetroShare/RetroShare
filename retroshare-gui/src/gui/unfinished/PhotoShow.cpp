/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include <QFile>
#include <QFileInfo>

#include "PhotoShow.h"
#include <retroshare/rspeers.h>
#include <retroshare/rsphoto.h>

#include <iostream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QMessageBox>
#include <QHeaderView>
#include <QTimer>


/** Constructor */
PhotoShow::PhotoShow(QWidget *parent)
: QWidget(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 // connect( ui.peerTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( peerTreeWidgetCustomPopupMenu( QPoint ) ) );
 // connect( ui.photoTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( photoTreeWidgetCustomPopupMenu( QPoint ) ) );

 // connect( ui.peerTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem * , QTreeWidgetItem * ) ), this, SLOT( updatePhotoList( ) ) );

  //connect( ui.photoTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem * , QTreeWidgetItem * ) ), this, SLOT( displayPhoto( ) ) );
  //connect( ui.addButton, SIGNAL( clicked( ) ), this, SLOT( addPhotos( ) ) );
}

void PhotoShow::photoCustomPopupMenu( QPoint point )
{

//      QMenu contextMnu( this );
//      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );
//
//      QAction *rm = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Remove" ), this );
//      connect( rm , SIGNAL( triggered() ), this, SLOT( removePhoto() ) );
//
//      contextMnu.clear();
//      contextMnu.addAction(rm);
//      contextMnu.addSeparator();
//      contextMnu.exec( mevent->globalPos() );
}

void PhotoShow::updatePhotoShow()
{
	std::cerr << "PhotoShow::updatePhotoShow()" << std::endl;

	if (mDoShow)
	{
		std::cerr << "PhotoShow::updatePhotoShow() (Show) pid: " << mPeerId << " sid: " << mShowId << std::endl;
	}
	else
	{
		std::cerr << "PhotoShow::updatePhotoShow() (Image) pid: " << mPeerId << " photoId: " << mPhotoId << std::endl;
		updatePhoto(mPeerId, mPhotoId);
	}

}

void PhotoShow::updatePhoto(std::string pid, std::string photoId)
{
	RsPhotoDetails photoDetail;
	std::cerr << "PhotoShow::updatePhoto() pid: ";
	std::cerr << pid << " photoId: " << photoId;
	std::cerr << std::endl;

	if (!rsPhoto->getPhotoDetails(pid, photoId, photoDetail))
	{
		std::cerr << "PhotoShow::updatePhotoShow() BAD Photo Id";
		std::cerr << std::endl;
		return;
	}

	ui.photoLabel->clear();

	if (photoDetail.isAvailable)
	{
		QPixmap qpp(QString::fromStdString(photoDetail.path));
		QSize diaSize = ui.photoLabel->size();
		int width = diaSize.width();
		ui.photoLabel->setPixmap(qpp.scaledToWidth(width));
	}
	else
	{
		ui.photoLabel->setText("Photo Not Available");
	}

	ui.commentLineEdit->setText(QString::fromStdWString(photoDetail.comment));
	ui.dateLineEdit->setText(QString::fromStdString(photoDetail.date));
	ui.locLineEdit->setText(QString::fromStdString(photoDetail.location));

	//ui.nameLineEdit->setText(QString::fromStdString(photoDetail.name));
	//ui.sizeLineEdit->setText(QString::number(photoDetail.size));
	//ui.peeridLineEdit->setText(QString::fromStdString(photoDetail.id));
	//ui.photoidLineEdit->setText(QString::fromStdString(photoDetail.hash));

}

void  PhotoShow::setPeerId(std::string id)
{
	mPeerId = id;
	return;
}

void  PhotoShow::setPhotoId(std::string id)
{
	mPhotoId = id;
	mDoShow = false;

	updatePhotoShow();
	return;
}

void  PhotoShow::setPhotoShow(std::string id)
{
	mShowId = id;
	mDoShow = true;

	updatePhotoShow();
	return;
}



