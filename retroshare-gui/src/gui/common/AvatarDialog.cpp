/*
 * Avatar Dialog
 *
 * Copyright 2015 by RetroShare Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <QBuffer>

#include "gui/common/AvatarDialog.h"
#include "gui/common/AvatarDefs.h"

#include "gui/gxs/GxsIdDetails.h"
#include "util/TokenQueue.h"
#include "util/misc.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include <iostream>


/** Constructor */
AvatarDialog::AvatarDialog(QWidget *parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);
	
	ui.headerFrame->setHeaderImage(QPixmap(":/images/no_avatar_70.png"));
	ui.headerFrame->setHeaderText(tr("Set your Avatar picture"));


	connect(ui.avatarButton, SIGNAL(clicked(bool)), this, SLOT(changeAvatar()));
	connect(ui.removeButton, SIGNAL(clicked(bool)), this, SLOT(removeAvatar()));
	
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(saveAvatar()));
	
	loadOwnAvatar();

}

AvatarDialog::~AvatarDialog()
{
}

void AvatarDialog::changeAvatar()
{
  QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load Avatar"), 128, 128);

  if (img.isNull())
    return;

  ui.avatarLabel->setPixmap(img) ;
  
}

void AvatarDialog::loadOwnAvatar()
{
    RsPeerDetails pd ;
    if (rsPeers->getPeerDetails(rsPeers->getOwnId(),pd)) {
      QPixmap avatar;
      AvatarDefs::getOwnAvatar(avatar);
      ui.avatarLabel->setPixmap(avatar);
      return;
    }
}

void AvatarDialog::removeAvatar()
{
   ui.avatarLabel->setPixmap(NULL);
}

void AvatarDialog::saveAvatar()
{	
	  const QPixmap *pixmap = ui.avatarLabel->pixmap();

    if (!pixmap->isNull())
    {
        QByteArray ba;
        QBuffer buffer(&ba);

        buffer.open(QIODevice::WriteOnly);
        pixmap->save(&buffer, "PNG"); // writes image into ba in PNG format

        rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()), ba.size()) ;	// last char 0 included.

    }

    close();
}

void AvatarDialog::setAvatar(const RsGxsImage &avatar)
{ 

}
 
void AvatarDialog::getAvatar(RsGxsImage &avatar)
{

}
