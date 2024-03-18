/*******************************************************************************
 * gui/TheWire/PulseReply.cpp                                                  *
 *                                                                             *
 * Copyright (c) 2020-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#include <QDateTime>
#include <QMessageBox>
#include <QMouseEvent>
#include <QBuffer>

#include "PulseReply.h"

#include <algorithm>
#include <iostream>

/** Constructor */

PulseReply::PulseReply(PulseViewHolder *holder, RsWirePulseSPtr pulse)
:PulseDataItem(holder, pulse)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );

	setup();

	if (mPulse) {
		showPulse();
	}

	widget_prefix->setVisible(false);
}

void PulseReply::setup()
{
	// connect(pushButton_tmpViewGroup, SIGNAL(clicked()), this, SLOT(actionViewGroup()));
	// connect(pushButton_tmpViewParent, SIGNAL(clicked()), this, SLOT(actionViewParent()));

	connect(followButton, SIGNAL(clicked()), this, SLOT(actionFollow()));
	// connect(toolButton_rate, SIGNAL(clicked()), this, SLOT(rate()));

	connect(toolButton_reply, SIGNAL(clicked()), this, SLOT(actionReply()));
	connect(toolButton_republish, SIGNAL(clicked()), this, SLOT(actionRepublish()));
	connect(toolButton_like, SIGNAL(clicked()), this, SLOT(actionLike()));
	connect(toolButton_view, SIGNAL(clicked()), this, SLOT(actionViewPulse()));
}

void PulseReply::showReplyLine(bool enable)
{
	line_replyLine->setVisible(enable);
}

// PulseDataInterface ===========
	// Group
void PulseReply::setHeadshot(const QPixmap &pixmap)
{
	label_headshot->setPixmap(pixmap);
}

void PulseReply::setGroupNameString(QString name)
{
	label_groupName->setText("@" + name);
}

void PulseReply::setAuthorString(QString name)
{
	label_authorName->setText(BoldString(name));
}

	// Msg
void PulseReply::setRefMessage(QString msg, uint32_t image_count)
{
	widget_message->setMessage(msg);
	widget_message->setRefImageCount(image_count);
}

void PulseReply::setMessage(RsWirePulseSPtr pulse)
{
	widget_message->setup(pulse);
}

void PulseReply::setDateString(QString date)
{
	label_date->setText(date);
}

	// Refs
void PulseReply::setLikesString(QString likes)
{
	label_likes->setText(likes);
}

void PulseReply::setRepublishesString(QString repub)
{
	label_republishes->setText(repub);
}

void PulseReply::setRepliesString(QString reply)
{
	label_replies->setText(reply);	
}

void PulseReply::setPulseStatus(PulseStatus status)
{
	widget_actions->setVisible(status == PulseStatus::FULL);
	widget_follow->setVisible(status != PulseStatus::FULL);
	followButton->setEnabled(status == PulseStatus::UNSUBSCRIBED);

	switch(status)
	{
		case PulseStatus::FULL:
			break;
		case PulseStatus::UNSUBSCRIBED:
			break;
		case PulseStatus::NO_GROUP:
			label_follow_msg->setText("Group unavailable");
			break;
		case PulseStatus::REF_MSG:
			label_follow_msg->setText("Full Pulse unavailable");
			break;
	}
}

void PulseReply::setReferenceString(QString ref)
{
	if (ref.size() == 0)
	{
		widget_reply_header->setVisible(false);
	}
	else
	{
		label_reference->setText(ref);
	}
}
// PulseDataInterface ===========

void PulseReply::mousePressEvent(QMouseEvent *event)
{
    // Check if the event is a left mouse button press
//    if (event->button() == Qt::LeftButton && (IS_MSG_UNREAD(mPulse->mMeta.mMsgStatus) || IS_MSG_NEW(mPulse->mMeta.mMsgStatus)))
    if (event->button() == Qt::LeftButton && (IS_MSG_UNPROCESSED(mPulse->mMeta.mMsgStatus)))

    {
        uint32_t token;
        // Perform some action when the left mouse button is pressed
        RsGxsGrpMsgIdPair msgPair = std::make_pair(mPulse->mMeta.mGroupId, mPulse->mMeta.mMsgId);

        rsWire->setMessageReadStatus(token, msgPair, true);
        std::cout << "Left mouse button pressed on PulseReply!" <<std::endl;
        // You can add your own custom code here
    }
    else{
        bool one = event->button() == Qt::LeftButton;
        std::cout << "the first condition:"<< one <<std::endl;
        one = IS_MSG_UNREAD(mPulse->mMeta.mMsgStatus);
        std::cout << "the second condition:"<< one <<std::endl;
        one = IS_MSG_NEW(mPulse->mMeta.mMsgStatus);
        std::cout << "the third condition:"<< one <<std::endl;
        one = IS_MSG_UNPROCESSED(mPulse->mMeta.mMsgStatus);
        std::cout << "the fourth condition:"<< one <<std::endl;
    }
    // Call the base class implementation to ensure proper event handling
    QWidget::mousePressEvent(event);
}


//void WireDialog::setAllMessagesReadDo(bool read, uint32_t &token)
//{
//    if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(subscribeFlags())) {
//        return;
//    }

//    foreach (RsWirePulseItem *item, mPostItems) {
//        RsGxsGrpMsgIdPair msgPair = std::make_pair(item->groupId(), item->messageId());

//        rsWire->setMessageReadStatus(token, msgPair, read);
//    }
//}

