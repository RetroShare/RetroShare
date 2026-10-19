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
#include <QStyle>

#include "PulseReply.h"
#include "gui/common/FilesDefs.h"

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

bool PulseReply::isUnread() const
{
	return IS_MSG_UNREAD(mPulse->mMeta.mMsgStatus) ;
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
	connect(readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool))); 

	// hided it takes to much space
	label_authorName->hide();

	applyFeedStyle();
}

// Twitter-like look: borderless card with a divider line, one-line header
// "Name  shortId · 5m", icon action row with counts (hidden when 0).
void PulseReply::applyFeedStyle()
{
	feedFrame->setFrameShape(QFrame::NoFrame);
	feedFrame->setAttribute(Qt::WA_Hover, true);

	QFont nameFont = label_groupName->font();
	nameFont.setBold(true);
	label_groupName->setFont(nameFont);

	// header on one line: name, id, date, then stretch, then read/new markers.
	horizontalLayout_12->setSpacing(4);
	horizontalLayout_12->removeWidget(label_date);
	horizontalLayout_12->insertWidget(horizontalLayout_12->indexOf(label_groupName) + 1, label_date);


	// actions: icon + count, compact row aligned left.
	WireStyleActionButton(toolButton_reply);
	WireStyleActionButton(toolButton_republish);
	WireStyleActionButton(toolButton_like);
	WireStyleActionButton(toolButton_view, false);

	toolButton_reply->setToolTip(tr("Reply"));
	toolButton_republish->setToolTip(tr("Republish"));
	toolButton_like->setToolTip(tr("Like"));
	toolButton_view->setToolTip(tr("Open thread"));

	toolButton_reply->setText(QString());
	toolButton_republish->setText(QString());
	toolButton_like->setText(QString());
	toolButton_view->setText(QString());

	label_replies->hide();
	label_republishes->hide();
	label_likes->hide();

	WireFitButtonText(followButton);

	widget_actions->setMinimumHeight(0);
	widget_actions->setMaximumWidth(425);
	plainFrame_GL->setAlignment(widget_actions, Qt::AlignLeft);

}

void PulseReply::showReferenceHeader(bool show)
{
	if (!show) {
		widget_reply_header->setVisible(false);
	}
}

void PulseReply::showReplyLine(bool enable)
{
	line_replyLine->setVisible(enable);
}

// PulseDataInterface ===========
	// Group
void PulseReply::setHeadshot(const QPixmap &pixmap)
{
	label_headshot->setPixmap(WireRoundPixmap(pixmap, 48));
}

void PulseReply::setGroupNameString(QString name)
{
	// name in bold (font), the short id + date are in label_date (muted color from qss).
	label_groupName->setTextFormat(Qt::PlainText);
	label_groupName->setText(name);
	label_groupName->setToolTip(QString::fromStdString(displayGroupId().toStdString()));
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
	// Wire names are not unique, so show the start of the group id next to the name.
	label_date->setTextFormat(Qt::PlainText);
	label_date->setText(QString("%1 %2 %3").arg(WireShortId(displayGroupId().toStdString()), QChar(0x00B7), date));
	label_date->setToolTip(mDateFull);
}

	// Refs
void PulseReply::setLikesString(QString likes)
{
	label_likes->setText(likes);
	toolButton_like->setText(WireCountText(likes));
}

void PulseReply::setRepublishesString(QString repub)
{
	label_republishes->setText(repub);
	toolButton_republish->setText(WireCountText(repub));
}

void PulseReply::setRepliesString(QString reply)
{
	label_replies->setText(reply);
	toolButton_reply->setText(WireCountText(reply));
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
    if (event->button() == Qt::LeftButton && IS_MSG_UNPROCESSED(mPulse->mMeta.mMsgStatus))
    {
        uint32_t token;
        RsGxsGrpMsgIdPair msgPair = std::make_pair(mPulse->mMeta.mGroupId, mPulse->mMeta.mMsgId);
        rsWire->setMessageReadStatus(token, msgPair, true);
    }
    QWidget::mousePressEvent(event);
}

void PulseReply::readToggled(bool checked)
{
    if (mInFill){
        return;
    }

    RsGxsGrpMsgIdPair msgPair = std::make_pair(mPulse->mMeta.mGroupId, mPulse->mMeta.mMsgId);
        
    // Use the wire service to update the backend
    rsWire->setMessageReadStatus(msgPair, isUnread()); 
}

void PulseReply::setReadStatus(bool isNew, bool isUnread)
{
	if (isNew)
		mPulse->mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_NEW;
	else
		mPulse->mMeta.mMsgStatus &= ~GXS_SERV::GXS_MSG_STATUS_GUI_NEW;

	// Update the Read Button Icon and State
	if (isUnread)
	{
		mPulse->mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		readButton->setChecked(true);
		readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
	}
	else
	{
		mPulse->mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		readButton->setChecked(false);
		readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png"));
	}

	// Update the New Label
	newLabel->setVisible(isNew);

	feedFrame->setProperty("new", isNew);
	feedFrame->style()->unpolish(feedFrame);
	feedFrame->style()->polish(feedFrame);
}
