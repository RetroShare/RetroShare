/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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

#include <rshare.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include "gui/notifyqt.h"
#include "gui/common/AvatarDefs.h"
#include "gui/common/AvatarDialog.h"

#include "AvatarWidget.h"
#include "ui_AvatarWidget.h"

#include <algorithm>

AvatarWidget::AvatarWidget(QWidget *parent) :
	QLabel(parent), ui(new Ui::AvatarWidget)
{
	ui->setupUi(this);

	mFlag.isOwnId = false;
//	mFlag.isGpg = false;
	defaultAvatar = ":/images/no_avatar_background.png";

	setFrameType(NO_FRAME);

	/* connect signals */
	connect(NotifyQt::getInstance(), SIGNAL(peerHasNewAvatar(const QString&)), this, SLOT(updateAvatar(const QString&)));
	connect(NotifyQt::getInstance(), SIGNAL(ownAvatarChanged()), this, SLOT(updateOwnAvatar()));
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

void AvatarWidget::mouseReleaseEvent(QMouseEvent */*event*/)
{
	if (!mFlag.isOwnId) {
		return;
	}

	AvatarDialog dialog(this);

	QPixmap avatar;
	AvatarDefs::getOwnAvatar(avatar, "");

	dialog.setAvatar(avatar);
	if (dialog.exec() == QDialog::Accepted) {
		QByteArray newAvatar;
		dialog.getAvatar(newAvatar);

		rsMsgs->setOwnAvatarData((unsigned char *)(newAvatar.data()), newAvatar.size()) ;	// last char 0 included.
	}
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
//    mPgpId = rsPeers->getGPGId(id) ;
//    mFlag.isGpg = false ;

    setPixmap(QPixmap());

    if (id.isNotSet()) {
        setEnabled(false);
    }

    refreshAvatarImage();
    refreshStatus();
}
void AvatarWidget::setOwnId(const RsGxsId& own_gxs_id)
{
    mFlag.isOwnId = true;

    setId(ChatId(own_gxs_id));
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
        uint32_t status ;

    if(mId.isNotSet())
        return ;

        if (mFlag.isOwnId)
        {
            if(mId.isPeerId())
            {
                StatusInfo statusInfo;
                rsStatus->getOwnStatus(statusInfo);
                status = statusInfo.status ;
            }
            else if(mId.isGxsId())
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
            else if(mId.isGxsId())
            {
                if(!rsMsgs->getDistantChatStatus(mId.toGxsId(),status))
                    status = RS_STATUS_OFFLINE ;
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
    if(mId.isPeerId() && mId.toPeerId() == RsPeerId(peerId.toStdString()))
        refreshAvatarImage() ;

    if(mId.isGxsId() && mId.toGxsId() == RsGxsId(peerId.toStdString()))
        refreshAvatarImage() ;
}
void AvatarWidget::refreshAvatarImage()
{
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
    else  if (mId.isGxsId())
    {
        QPixmap avatar;
        AvatarDefs::getAvatarFromGxsId(mId.toGxsId(), avatar, defaultAvatar);
        setPixmap(avatar);
        return;
    }
    else
        std::cerr << "WARNING: unhandled situation in AvatarWidget::refreshAvatarImage()" << std::endl;
}

void AvatarWidget::updateOwnAvatar()
{
    if (mFlag.isOwnId)
        refreshAvatarImage() ;
}

