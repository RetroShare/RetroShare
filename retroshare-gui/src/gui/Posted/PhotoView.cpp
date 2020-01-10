/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PhotoView.cpp                                *
 *                                                                             *
 * Copyright (C) 2020 by RetroShare Team       <retroshare.project@gmail.com>  *
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

#include "PhotoView.h"

#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

#include "gui/gxs/GxsIdDetails.h"
#include <retroshare/rsidentity.h>

/** Constructor */
PhotoView::PhotoView(QWidget *parent)
: QDialog(parent),
	ui(new Ui::PhotoView)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui->setupUi(this);

  ui->shareButton->hide();
}

/** Destructor */
PhotoView::~PhotoView()
{
	delete ui;
}

void PhotoView::setPixmap(const QPixmap& pixmap) 
{
	ui->photoLabel->setPixmap(pixmap);
}

void PhotoView::setTitle(const QString& text) 
{
	ui->titleLabel->setText(text);
}

void PhotoView::setName(const RsGxsId& authorID) 
{
	ui->nameLabel->setId(authorID);
	
	RsIdentityDetails idDetails ;
	rsIdentity->getIdDetails(authorID,idDetails);
		
	QPixmap pixmap ;

	if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
			pixmap = GxsIdDetails::makeDefaultIcon(authorID,GxsIdDetails::SMALL);
			
	ui->avatarWidget->setPixmap(pixmap);
}

void PhotoView::setTime(const QString& text) 
{
	ui->timeLabel->setText(text);
}

