/*******************************************************************************
 * plugins/VOIP/gui/VOIPToasterItem.cpp                                        *
 *                                                                             *
 * Copyright (C) 2015 by Retroshare Team <retroshare.project@gmail.com>        *
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
			//acceptButton->setIcon(QIcon("://images/call-start.png"));
			acceptButton->setText(tr("Answer"));
		break;
		case VideoCall:
			acceptButton->setIcon(QIcon("://images/video-icon-on.png"));
			acceptButton->setText(tr("Answer with video"));
		break;
		default:
			ChatDialog::chatFriend(ChatId(mPeerId));
	}

	connect(acceptButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));
	connect(declineButton, SIGNAL(clicked()), SLOT(declineButtonSlot()));
	connect(closeButton, SIGNAL(clicked()), SLOT(hide()));

	/* set informations */
    // emoticons disabled because of crazy cost
	//textLabel->setText(RsHtml().formatText(NULL, msg, RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_CLEANSTYLE));
	textLabel->setText(RsHtml().formatText(NULL, msg, RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_CLEANSTYLE));
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

void VOIPToasterItem::declineButtonSlot()
{
	switch (mType){
		case AudioCall:
			VOIPGUIHandler::HangupAudioCall(mPeerId);
		break;
		case VideoCall:
			VOIPGUIHandler::HangupVideoCall(mPeerId);
		break;
		default:
			ChatDialog::chatFriend(ChatId(mPeerId));
	}
	hide();
}

#ifdef VOIPTOASTERNOTIFY_ALL
void VOIPToasterItem::voipAcceptReceived(const RsPeerId &, int )
{
}

void VOIPToasterItem::voipBandwidthInfoReceived(const RsPeerId &, int )
{
}

void VOIPToasterItem::voipDataReceived(const RsPeerId &)
{
}

void VOIPToasterItem::voipHangUpReceived(const RsPeerId &, int )
{
}

void VOIPToasterItem::voipInvitationReceived(const RsPeerId &, int )
{
}

void VOIPToasterItem::voipAudioCallReceived(const RsPeerId &)
{
}

void VOIPToasterItem::voipVideoCallReceived(const RsPeerId &)
{
}
#endif
