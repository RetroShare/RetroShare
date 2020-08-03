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

#include "PulseTopLevel.h"

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

void PulseTopLevel::setup()
{
	connect(pushButton_tmpViewGroup, SIGNAL(clicked()), this, SLOT(actionViewGroup()));
	connect(pushButton_tmpViewParent, SIGNAL(clicked()), this, SLOT(actionViewParent()));

	// connect(toolButton_follow, SIGNAL(clicked()), this, SLOT(follow()));
	// connect(toolButton_rate, SIGNAL(clicked()), this, SLOT(rate()));

	connect(toolButton_reply, SIGNAL(clicked()), this, SLOT(actionReply()));
	connect(toolButton_republish, SIGNAL(clicked()), this, SLOT(actionRepublish()));
	connect(toolButton_like, SIGNAL(clicked()), this, SLOT(actionLike()));
	connect(toolButton_view, SIGNAL(clicked()), this, SLOT(actionViewPulse()));
}

void PulseTopLevel::setRefMessage(QString msg, uint32_t image_count)
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
	label_headshot->setPixmap(pixmap);
}

void PulseTopLevel::setGroupNameString(QString name)
{
	label_groupName->setText("@" + name);
}

void PulseTopLevel::setAuthorString(QString name)
{
	label_authorName->setText(BoldString(name));
}

void PulseTopLevel::setDateString(QString date)
{
	label_date->setText(date);
}

void PulseTopLevel::setLikesString(QString likes)
{
	label_extra_likes->setText(BoldString(likes));
	label_likes->setText(likes);
}

void PulseTopLevel::setRepublishesString(QString repub)
{
	label_extra_republishes->setText(BoldString(repub));
	label_republishes->setText(repub);
}

void PulseTopLevel::setRepliesString(QString reply)
{
	label_extra_replies->setText(BoldString(reply));
	label_replies->setText(reply);
}
	
void PulseTopLevel::showResponseStats(bool enable)
{
	widget_replies->setVisible(enable);
	widget_actions->setVisible(enable);
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
	}
}
	
void PulseTopLevel::mousePressEvent(QMouseEvent *event)
{
}


