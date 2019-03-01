/*******************************************************************************
 * gui/common/AvatarWidget.cpp                                                 *
 *                                                                             *
 * Copyright (C) 2012, Robert Fernie <retroshare.project@gmail.com>            *
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

#include <rshare.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <QBuffer>
#include <QMenu>
#include <QMouseEvent>

#include "gui/notifyqt.h"
#include "gui/common/AvatarDefs.h"

#include "AvatarWidget.h"
#include "ui_AvatarWidget.h"
#include "util/misc.h"

#include <algorithm>

//#define DEBUG_AVATAR_GUI 1

AvatarWidget::AvatarWidget(QWidget *parent) : QLabel(parent), ui(new Ui::AvatarWidget)
{
	ui->setupUi(this);

	mFlag.isOwnId = false;
	defaultAvatar = ":/images/no_avatar_background.png";
	mPeerState = RS_STATUS_OFFLINE ;

	setFrameType(NO_FRAME);

    setContextMenuPolicy(Qt::CustomContextMenu);

	/* connect signals */
	connect(NotifyQt::getInstance(), SIGNAL(peerHasNewAvatar(const QString&)), this, SLOT(updateAvatar(const QString&)));
	connect(NotifyQt::getInstance(), SIGNAL(ownAvatarChanged()), this, SLOT(updateOwnAvatar()));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
}

AvatarWidget::~AvatarWidget()
{
	delete ui;
}

QString AvatarWidget::frameState()
{
	switch (mFrameType)
	{
	case NO_FRAME:
		return "NOTHING";
	case NORMAL_FRAME:
		return "NORMAL";
	case STATUS_FRAME:
		switch (mPeerState)
		{
		case RS_STATUS_OFFLINE:
			return "OFFLINE";
		case RS_STATUS_INACTIVE:
			return "INACTIVE";
		case RS_STATUS_ONLINE:
			return "ONLINE";
		case RS_STATUS_AWAY:
			return "AWAY";
		case RS_STATUS_BUSY:
			return "BUSY";
		}
	}
	return "NOTHING";
}

void AvatarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!mFlag.isOwnId) {
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        selectAvatar();
        return;
    }
}

void AvatarWidget::showContextMenu(const QPoint &pos)
{
    QMenu contextMenu(this);

    QAction actionRemove(tr("Remove avatar"), this);
    connect(&actionRemove, SIGNAL(triggered()), this, SLOT(removeAvatar()));
    contextMenu.addAction(&actionRemove);

    contextMenu.exec(mapToGlobal(pos));
}

void AvatarWidget::selectAvatar()
{
    QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load Avatar"), 128, 128);

    if (img.isNull())
    {
        return;
    }

    QByteArray newAvatar;
    QBuffer buffer(&newAvatar);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");

    rsMsgs->setOwnAvatarData((unsigned char *)(newAvatar.data()), newAvatar.size()) ;
}

void AvatarWidget::removeAvatar()
{
    QByteArray newAvatar;
    newAvatar.clear(); /* XXX: is clear() needed? */

    rsMsgs->setOwnAvatarData((unsigned char *)(newAvatar.data()), newAvatar.size()) ;
}

void AvatarWidget::setFrameType(FrameType type)
{
	mFrameType = type;

	switch (mFrameType) {
	case NO_FRAME:
		disconnect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(QString,int)), this, SLOT(updateStatus(const QString&, int)));
		break;
	case NORMAL_FRAME:
		disconnect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(QString,int)), this, SLOT(updateStatus(const QString&, int)));
		break;
	case STATUS_FRAME:
		connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(QString,int)), this, SLOT(updateStatus(const QString&, int)));
		break;
	}

    //refreshAvatarImage();
    refreshStatus();
    Rshare::refreshStyleSheet(this, false);
}
void AvatarWidget::setId(const ChatId &id)
{
    mId = id;
    mGxsId.clear();

    setPixmap(QPixmap());

    if (id.isNotSet()) {
        setEnabled(false);
    }
    
    refreshAvatarImage();
    refreshStatus();
}

void AvatarWidget::setGxsId(const RsGxsId &id)
{
    mId = ChatId();
    mGxsId = id;

    setPixmap(QPixmap());

    if (id.isNull()) {
        setEnabled(false);
    }

    refreshAvatarImage();
    refreshStatus();
}

void AvatarWidget::setOwnId()
{
    mFlag.isOwnId = true;
    setToolTip(tr("Click to change your avatar"));

    setId(ChatId(rsPeers->getOwnId()));
}

void AvatarWidget::setDefaultAvatar(const QString &avatar_file_name)
{
    defaultAvatar = avatar_file_name;
    refreshAvatarImage();
}

void AvatarWidget::refreshStatus()
{
    switch (mFrameType)
    {
    case NO_FRAME:
    case NORMAL_FRAME:
    {
        Rshare::refreshStyleSheet(this, false);
        break;
    }
    case STATUS_FRAME:
    {
        uint32_t status = 0;

        if (mId.isNotSet())
            return;

        if (mFlag.isOwnId)
        {
            if(mId.isPeerId())
            {
                StatusInfo statusInfo;
                rsStatus->getOwnStatus(statusInfo);
                status = statusInfo.status ;
            }
            else if(mId.isDistantChatId())
                status = RS_STATUS_ONLINE ;
            else
            {
                std::cerr << "Unhandled chat id type in AvatarWidget::refreshStatus()" << std::endl;
                return ;
            }
        }
        else
        {
            // No check of return value. Non existing status info is handled as offline.
            if(mId.isPeerId())
            {
                StatusInfo statusInfo;
                rsStatus->getStatus(mId.toPeerId(), statusInfo);
                status = statusInfo.status ;
            }
            else if(mId.isDistantChatId())
	    {
		    DistantChatPeerInfo dcpinfo ;

		    if(rsMsgs->getDistantChatStatus(mId.toDistantChatId(),dcpinfo))
			    status = dcpinfo.status ;
		    else
			    std::cerr << "(EE) cannot get distant chat status for ID=" << mId.toDistantChatId() << std::endl;
	    }
            else
            {
                std::cerr << "Unhandled chat id type in AvatarWidget::refreshStatus()" << std::endl;
                return ;
            }
        }
        updateStatus(status);
        break;
    }
    }
}

void AvatarWidget::updateStatus(const QString& peerId, int status)
{
        if (mId.isPeerId() && mId.toPeerId() == RsPeerId(peerId.toStdString()))
        updateStatus(status) ;
}

void AvatarWidget::updateStatus(int status)
{
    if (mFrameType != STATUS_FRAME)
		return;

    mPeerState = status;

    setEnabled(((uint32_t) status == RS_STATUS_OFFLINE) ? false : true);
    Rshare::refreshStyleSheet(this, false);
}

void AvatarWidget::updateAvatar(const QString &peerId)
{
	if(mId.isPeerId()){
		if(mId.toPeerId() == RsPeerId(peerId.toStdString()))
			refreshAvatarImage() ;
		//else not mId so pass through
	} else if(mId.isDistantChatId()) {
			if (mId.toDistantChatId() == DistantChatPeerId(peerId.toStdString()))
				refreshAvatarImage() ;
			//else not mId so pass through
	} 
#ifdef DEBUG_AVATAR_GUI
    else
		std::cerr << "(EE) cannot update avatar. mId has unhandled type." << std::endl;
#endif
}

void AvatarWidget::refreshAvatarImage()
{
    if (mGxsId.isNull()==false)
    {
        QPixmap avatar;

        AvatarDefs::getAvatarFromGxsId(mGxsId, avatar, defaultAvatar);
        setPixmap(avatar);
        return;
    }
    if (mId.isNotSet())
    {
        QPixmap avatar(defaultAvatar);
        setPixmap(avatar);
        return;
    }
    else  if (mFlag.isOwnId && mId.isPeerId())
    {
        QPixmap avatar;
        AvatarDefs::getOwnAvatar(avatar);
        setPixmap(avatar);
        return;
    }
    else  if (mId.isPeerId())
    {
        QPixmap avatar;
        AvatarDefs::getAvatarFromSslId(mId.toPeerId(), avatar, defaultAvatar);
        setPixmap(avatar);
        return;
    }
    else  if (mId.isDistantChatId())
    {
	    QPixmap avatar;

	    DistantChatPeerInfo dcpinfo ;

	    if(rsMsgs->getDistantChatStatus(mId.toDistantChatId(),dcpinfo))
	    {
		    if(mFlag.isOwnId)
			    AvatarDefs::getAvatarFromGxsId(dcpinfo.own_id, avatar, defaultAvatar);
		    else
			    AvatarDefs::getAvatarFromGxsId(dcpinfo.to_id, avatar, defaultAvatar);
		    setPixmap(avatar);
		    return;
	    }
    }
    else
        std::cerr << "WARNING: unhandled situation in AvatarWidget::refreshAvatarImage()" << std::endl;
}

void AvatarWidget::updateOwnAvatar()
{
    if (mFlag.isOwnId)
        refreshAvatarImage() ;
}

