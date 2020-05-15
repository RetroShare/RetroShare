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

	connect(toolButton_follow, SIGNAL(clicked()), this, SLOT(actionFollow()));
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
	//, image_count);
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

void PulseReply::showResponseStats(bool enable)
{
	widget_actions->setVisible(enable);
	widget_follow->setVisible(!enable);
}

void PulseReply::setReferenceString(QString ref)
{
	if (ref.size() == 0)
	{
		// appear to have duplicated here....
		// widget_prefix->setVisible(false);
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
}


