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
#include "util/misc.h"

#include "AvatarWidget.h"
#include "ui_AvatarWidget.h"

#include <algorithm>

AvatarWidget::AvatarWidget(QWidget *parent) :
	QLabel(parent), ui(new Ui::AvatarWidget)
{
	ui->setupUi(this);

	mFlag.isOwnId = false;
	mFlag.isGpg = false;
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
	if (mFlag.isOwnId) {
		QByteArray ba;
		if (misc::getOpenAvatarPicture(this, ba)) {
			rsMsgs->setOwnAvatarData((unsigned char*)(ba.data()), ba.size()); // last char 0 included.
		}
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

	refreshStatus();
	updateAvatar(QString::fromStdString(mId));
	Rshare::refreshStyleSheet(this, false);
}

void AvatarWidget::setId(const std::string &id, bool isGpg)
{
	mId = id;
	mFlag.isGpg = isGpg;

	if (mId == rsPeers->getOwnId()) {
		mFlag.isOwnId = true;
		setToolTip(tr("Click to change your avatar"));
	}

	setPixmap(QPixmap());

	if (mId.empty()) {
		setEnabled(false);
	}

	refreshStatus();
	updateAvatar(QString::fromStdString(mId));
}

void AvatarWidget::setOwnId()
{
	setId(rsPeers->getOwnId(), false);
}

void AvatarWidget::setDefaultAvatar(const QString &avatar)
{
	defaultAvatar = avatar;
	updateAvatar(QString::fromStdString(mId));
}

void AvatarWidget::refreshStatus()
{
	switch (mFrameType) {
	case NO_FRAME:
	case NORMAL_FRAME:
	{
		Rshare::refreshStyleSheet(this, false);
		break;
	}
	case STATUS_FRAME:
	{
		StatusInfo statusInfo;

		if (mFlag.isOwnId) {
			rsStatus->getOwnStatus(statusInfo);
		} else {
			// No check of return value. Non existing status info is handled as offline.
			rsStatus->getStatus(mId, statusInfo);
		}
		updateStatus(QString::fromStdString(statusInfo.id), statusInfo.status);
		break;
	}
	}
}

void AvatarWidget::updateStatus(const QString peerId, int status)
{
	if (mFrameType != STATUS_FRAME) {
		return;
	}

	if (mId.empty()) {
		mPeerState = status;
		Rshare::refreshStyleSheet(this, false);
	} else {
		/* set style for status */
		if (mId == peerId.toStdString()) {
			// the peers status has changed
			mPeerState = status;
			setEnabled(((uint32_t) status == RS_STATUS_OFFLINE) ? false : true);
			Rshare::refreshStyleSheet(this, false);
		}
	}
}

void AvatarWidget::updateAvatar(const QString &peerId)
{
	if (mId.empty()) {
		QPixmap avatar(defaultAvatar);
		setPixmap(avatar);
		return;
	}

	if (mFlag.isOwnId) {
		QPixmap avatar;
		AvatarDefs::getOwnAvatar(avatar);
		setPixmap(avatar);
		return;
	}

	if (mFlag.isGpg) {
		if (mId == peerId.toStdString()) {
			/* called from AvatarWidget with gpg id */
			QPixmap avatar;
			AvatarDefs::getAvatarFromGpgId(mId, avatar, defaultAvatar);
			setPixmap(avatar);
			return;
		}

		/* Is this one of the ssl ids of the gpg id ? */
		std::list<std::string> sslIds;
		if (rsPeers->getAssociatedSSLIds(mId, sslIds) == false) {
			return;
		}

		if (std::find(sslIds.begin(), sslIds.end(), peerId.toStdString()) != sslIds.end()) {
			/* One of the ssl ids of the gpg id */
			QPixmap avatar;
			AvatarDefs::getAvatarFromGpgId(mId, avatar, defaultAvatar);
			setPixmap(avatar);
		}

		return;
	}

	if (mId == peerId.toStdString()) {
		QPixmap avatar;
		AvatarDefs::getAvatarFromSslId(mId, avatar, defaultAvatar);
		setPixmap(avatar);
		return;
	}
}

void AvatarWidget::updateOwnAvatar()
{
	if (mFlag.isOwnId) {
		updateAvatar(QString::fromStdString(mId));
	}
}
