/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015 RetroShare Team
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

/*VOIP*/
#include "VOIPToasterItem.h"
#include "VOIPGUIHandler.h"

/*libRetroshare*/
#include <retroshare/rspeers.h>

/*Retroshare-Gui*/
#include "gui/chat/ChatDialog.h"
#include "gui/notifyqt.h"
#include "util/HandleRichText.h"

VOIPToasterItem::VOIPToasterItem(const RsPeerId &peer_id, const QString &msg, const voipToasterItem_Type type)
  : QWidget(NULL), mPeerId(peer_id), mMsg(msg), mType(type)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

	switch (mType){
		case AudioCall:
			toasterButton->setIcon(QIcon("://images/call-start.png"));
			toasterButton->setText(tr("Answer"));
		break;
		case VideoCall:
			toasterButton->setIcon(QIcon("://images/video-icon-on.png"));
			toasterButton->setText(tr("Answer with video"));
		break;
		default:
			ChatDialog::chatFriend(ChatId(mPeerId));
	}

	connect(toasterButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));
	connect(closeButton, SIGNAL(clicked()), SLOT(hide()));

	/* set informations */
	textLabel->setText(RsHtml().formatText(NULL, msg, RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_CLEANSTYLE));
	//toasterLabel->setText(QString::fromUtf8(rsPeers->getPeerName(mPeerId).c_str()));
	avatarWidget->setFrameType(AvatarWidget::STATUS_FRAME);
	avatarWidget->setId(ChatId(mPeerId));
}

VOIPToasterItem::~VOIPToasterItem()
{
}

void VOIPToasterItem::chatButtonSlot()
{
	switch (mType){
		case AudioCall:
			VOIPGUIHandler::AnswerAudioCall(mPeerId);
		break;
		case VideoCall:
			VOIPGUIHandler::AnswerVideoCall(mPeerId);
		break;
		default:
			ChatDialog::chatFriend(ChatId(mPeerId));
	}
	hide();
}

void VOIPToasterItem::voipAcceptReceived(const RsPeerId &)
{
}

void VOIPToasterItem::voipBandwidthInfoReceived(const RsPeerId &, int )
{
}

void VOIPToasterItem::voipDataReceived(const RsPeerId &)
{
}

void VOIPToasterItem::voipHangUpReceived(const RsPeerId &)
{
}

void VOIPToasterItem::voipInvitationReceived(const RsPeerId &)
{
}

void VOIPToasterItem::voipAudioCallReceived(const RsPeerId &)
{
}

void VOIPToasterItem::voipVideoCallReceived(const RsPeerId &)
{
}

