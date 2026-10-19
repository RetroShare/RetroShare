/*******************************************************************************
 * gui/TheWire/PulseTopLevel.cpp                                               *
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

#include "PulseTopLevel.h"
#include "gui/common/FilesDefs.h"

#include <algorithm>
#include <iostream>

/** Constructor */

PulseTopLevel::PulseTopLevel(PulseViewHolder *holder, RsWirePulseSPtr pulse)
:PulseDataItem(holder, pulse)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );
	setup();

	if (mPulse) {
		showPulse();
	}

}

bool PulseTopLevel::isUnread() const
{
	return IS_MSG_UNREAD(mPulse->mMeta.mMsgStatus) ;
}

void PulseTopLevel::setup()
{
	connect(toolButton_viewGroup, SIGNAL(clicked()), this, SLOT(actionViewGroup()));
	connect(toolButton_viewParent, SIGNAL(clicked()), this, SLOT(actionViewParent()));
	connect(followButton, SIGNAL(clicked()), this, SLOT(actionFollow()));
	connect(toolButton_followParent, SIGNAL(clicked()), this, SLOT(actionFollowParent()));
	// connect(toolButton_rate, SIGNAL(clicked()), this, SLOT(rate()));

	connect(toolButton_reply, SIGNAL(clicked()), this, SLOT(actionReply()));
	connect(toolButton_republish, SIGNAL(clicked()), this, SLOT(actionRepublish()));
	connect(toolButton_like, SIGNAL(clicked()), this, SLOT(actionLike()));
	connect(toolButton_view, SIGNAL(clicked()), this, SLOT(actionViewPulse()));
	connect(toolButton_copyLink, SIGNAL(clicked()), this, SLOT(actionCopyLink()));
	connect(readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool))); 

	applyFeedStyle();
}

// Twitter-like detail view of a single pulse.
void PulseTopLevel::applyFeedStyle()
{
	plainFrame->setFrameShape(QFrame::NoFrame);

	WireStyleActionButton(toolButton_reply);
	WireStyleActionButton(toolButton_republish);
	WireStyleActionButton(toolButton_like);
	WireStyleActionButton(toolButton_view, false);
	WireStyleActionButton(toolButton_copyLink, false);

	toolButton_reply->setToolTip(tr("Reply"));
	toolButton_republish->setToolTip(tr("Republish"));
	toolButton_like->setToolTip(tr("Like"));
	toolButton_view->setToolTip(tr("Open thread"));
	toolButton_copyLink->setToolTip(tr("Copy link"));

	toolButton_reply->setText(QString());
	toolButton_republish->setText(QString());
	toolButton_like->setText(QString());
	toolButton_view->setText(QString());
	toolButton_copyLink->setText(QString());

	label_replies->hide();
	label_republishes->hide();
	label_likes->hide();

	widget_actions->setMinimumHeight(0);
	widget_actions->setMaximumWidth(425);
	plainFrame_VL->setAlignment(widget_actions, Qt::AlignLeft);


}

void PulseTopLevel::setRefMessage(QString /*msg*/, uint32_t /*image_count*/)
{
	// This should never happen.
	//widget_message->setRefMessage(msg, image_count);
}

void PulseTopLevel::setMessage(RsWirePulseSPtr pulse)
{
	widget_message->setup(pulse);
}

// Set UI elements.
void PulseTopLevel::setHeadshot(const QPixmap &pixmap)
{
	label_headshot->setPixmap(WireRoundPixmap(pixmap, 48));
}

void PulseTopLevel::setGroupNameString(QString name)
{
	// Name in bold, the start of the group id below it (names are not unique).
	RsGxsGroupId groupId = displayGroupId();

	label_authorName->setText(BoldString(name.toHtmlEscaped()));
	label_groupName->setTextFormat(Qt::PlainText);
	label_groupName->setText(WireShortId(groupId.toStdString()));
	label_groupName->setToolTip(QString::fromStdString(groupId.toStdString()));
}

void PulseTopLevel::setAuthorString(QString name)
{
	label_authorName->setToolTip(tr("Author: %1").arg(name));
}

void PulseTopLevel::setDateString(QString /*date*/)
{
	// detail view shows the full date, like Twitter.
	label_date->setTextFormat(Qt::PlainText);
	label_date->setText(mDateFull);
}

void PulseTopLevel::setLikesString(QString likes)
{
	label_extra_likes->setText(BoldString(likes));
	label_likes->setText(likes);
	toolButton_like->setText(WireCountText(likes));
}

void PulseTopLevel::setRepublishesString(QString repub)
{
	label_extra_republishes->setText(BoldString(repub));
	label_republishes->setText(repub);
	toolButton_republish->setText(WireCountText(repub));
}

void PulseTopLevel::setRepliesString(QString reply)
{
	label_extra_replies->setText(BoldString(reply));
	label_replies->setText(reply);
	toolButton_reply->setText(WireCountText(reply));
}
	
void PulseTopLevel::setPulseStatus(PulseStatus status)
{
	widget_replies->setVisible(true); // this is only reachable if we have ORIG so show always.
	widget_actions->setVisible(status == PulseStatus::FULL);
	widget_actionsFollow->setVisible(status == PulseStatus::UNSUBSCRIBED);
}

void PulseTopLevel::setReferenceString(QString ref)
{
	if (ref.size() == 0)
	{
		widget_prefix->setVisible(false);
	}
	else
	{
		label_reference->setText(ref);

		// set ref icon
		if (mPulse->mPulseType & WIRE_PULSE_TYPE_REPUBLISH) {
            label_reficon->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/retweet.png"));
		} else {
            label_reficon->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/reply.png"));
		}
	}

	if (mPulse->mRefGroupPtr) {
		if (mPulse->mRefGroupPtr->mMeta.mSubscribeFlags &
				(GXS_SERV::GROUP_SUBSCRIBE_ADMIN |
				 GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)) {
			toolButton_viewParent->setVisible(true);
			toolButton_followParent->setVisible(false);
		} else {
			toolButton_viewParent->setVisible(false);
			toolButton_followParent->setVisible(true);
		}
	} else {
		toolButton_viewParent->setVisible(false);
		toolButton_followParent->setVisible(false);
	}
}
	
void PulseTopLevel::mousePressEvent(QMouseEvent */*event*/)
{
}

void PulseTopLevel::readToggled(bool /*checked*/)
{
    if (mInFill) return;

    RsGxsGrpMsgIdPair msgPair = std::make_pair(mPulse->mMeta.mGroupId, mPulse->mMeta.mMsgId);
    rsWire->setMessageReadStatus(msgPair, isUnread());
}

void PulseTopLevel::setReadStatus(bool isNew, bool isUnread)
{

	// Update the Read Button Icon and State
	if (isUnread)
	{
		readButton->setChecked(true);
		readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
	}
	else
	{
		readButton->setChecked(false);
		readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png"));
	}

	// Update the New Label
	newLabel->setVisible(isNew);

    // leave it empty or just handle the frame styling:
    /*plainFrame->setProperty("new", isNew);
    plainFrame->style()->unpolish(plainFrame);
    plainFrame->style()->polish(plainFrame);*/
}


