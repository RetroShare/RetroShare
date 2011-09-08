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
	QWidget(parent), ui(new Ui::AvatarWidget)
{
	ui->setupUi(this);

	mFlag.isOwnId = false;
	mFlag.isGpg = false;

	setFrameType(NO_FRAME);

	/* connect signals */
	connect(NotifyQt::getInstance(), SIGNAL(peerHasNewAvatar(const QString&)), this, SLOT(updateAvatar(const QString&)));
	connect(NotifyQt::getInstance(), SIGNAL(ownAvatarChanged()), this, SLOT(updateOwnAvatar()));
}

AvatarWidget::~AvatarWidget()
{
	delete ui;
}

static bool isSmall(const QSize &size)
{
	if (size.width() <= 70 && size.height() <= 70) {
		return true;
	}

	return false;
}

void AvatarWidget::resizeEvent(QResizeEvent *event)
{
	if (mFrameType == NO_FRAME) {
		return;
	}

	QSize widgetSize = size();
	QSize avatarSize;
	if (isSmall(widgetSize)) {
		avatarSize = QSize(widgetSize.width() * 50 / 70, widgetSize.height() * 50 / 70);
	} else {
		avatarSize = QSize(widgetSize.width() * 96 / 116, widgetSize.height() * 96 / 116);
	}
	int x = (widgetSize.width() - avatarSize.width()) / 2;
	int y = (widgetSize.height() - avatarSize.height()) / 2;
	ui->avatarFrameLayout->setContentsMargins(x, y, x, y);

	refreshStatus();
}

void AvatarWidget::mouseReleaseEvent(QMouseEvent *event)
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
		ui->avatarFrameLayout->setContentsMargins(0, 0, 0, 0);
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
}

void AvatarWidget::setId(const std::string &id, bool isGpg)
{
	mId = id;
	mFlag.isGpg = isGpg;

	if (mId == rsPeers->getOwnId()) {
		mFlag.isOwnId = true;
		ui->avatar->setToolTip(tr("Click to change your avatar"));
	}

	ui->avatar->setPixmap(QPixmap());

	if (mId.empty()) {
		ui->avatar->setEnabled(false);
	}

	refreshStatus();
	updateAvatar(QString::fromStdString(mId));
}

void AvatarWidget::setOwnId()
{
	setId(rsPeers->getOwnId(), false);
}

void AvatarWidget::refreshStatus()
{
	switch (mFrameType) {
	case NO_FRAME:
		ui->avatarFrame->setStyleSheet("");
		break;
	case NORMAL_FRAME:
		ui->avatarFrame->setStyleSheet(isSmall(size()) ? "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-70.png); }" : "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-116.png); }");
		break;
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
		}
		break;
	}
}

static QString getStatusFrame(bool small, int status)
{
	if (small) {
		switch (status) {
		case RS_STATUS_OFFLINE:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-offline-70.png); }";
		case RS_STATUS_INACTIVE:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-idle-70.png); }";
		case RS_STATUS_ONLINE:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-online-70.png); }";
		case RS_STATUS_AWAY:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-away-70.png); }";
		case RS_STATUS_BUSY:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-busy-70.png); }";
		}

		return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-70.png); }";
	} else {
		switch (status) {
		case RS_STATUS_OFFLINE:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-offline-116.png); }";
		case RS_STATUS_INACTIVE:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-away-116.png); }";
		case RS_STATUS_ONLINE:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-online-116.png); }";
		case RS_STATUS_AWAY:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-away-116.png); }";
		case RS_STATUS_BUSY:
			return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-busy-116.png); }";
		}

		return "QFrame#avatarFrame{ border-image:url(:/images/avatarstatus-bg-116.png); }";
	}

	return "";
}

void AvatarWidget::updateStatus(const QString peerId, int status)
{
	if (mFrameType != STATUS_FRAME) {
		return;
	}

	if (mId.empty()) {
		ui->avatarFrame->setStyleSheet(getStatusFrame(isSmall(size()), RS_STATUS_OFFLINE));
		return;
	}

	/* set style for status */
	if (mId == peerId.toStdString()) {
		// the peers status has changed

		ui->avatarFrame->setStyleSheet(getStatusFrame(isSmall(size()), status));

		ui->avatar->setEnabled(((uint32_t) status == RS_STATUS_OFFLINE) ? false : true);

		return;
	}

	// ignore status change
}

void AvatarWidget::updateAvatar(const QString &peerId)
{
	QString defaultAvatar = ":/images/no_avatar_background.png";

	if (mId.empty()) {
		QPixmap avatar(defaultAvatar);
		ui->avatar->setPixmap(avatar);
		return;
	}

	if (mFlag.isOwnId) {
		QPixmap avatar;
		AvatarDefs::getOwnAvatar(avatar);
		ui->avatar->setPixmap(avatar);
		return;
	}

	if (mFlag.isGpg) {
		if (mId == peerId.toStdString()) {
			/* called from AvatarWidget with gpg id */
			QPixmap avatar;
			AvatarDefs::getAvatarFromGpgId(mId, avatar, defaultAvatar);
			ui->avatar->setPixmap(avatar);
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
			ui->avatar->setPixmap(avatar);
		}

		return;
	}

	if (mId == peerId.toStdString()) {
		QPixmap avatar;
		AvatarDefs::getAvatarFromSslId(mId, avatar, defaultAvatar);
		ui->avatar->setPixmap(avatar);
		return;
	}
}

void AvatarWidget::updateOwnAvatar()
{
	if (mFlag.isOwnId) {
		updateAvatar(QString::fromStdString(mId));
	}
}
