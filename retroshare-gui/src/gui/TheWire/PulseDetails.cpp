/*******************************************************************************
 * gui/TheWire/PulseDetails.cpp                                                *
 *                                                                             *
 * Copyright (c) 2020 Robert Fernie   <retroshare.project@gmail.com>           *
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

#include "PulseDetails.h"

#include "util/DateTime.h"

#include <algorithm>
#include <iostream>

/** Constructor */
PulseDetails::PulseDetails(PulseHolder *actions, RsWirePulse *pulse, std::string &groupName,
		std::map<rstime_t, RsWirePulse *> replies)
:QWidget(NULL), mActions(actions), mPulse(*pulse), mGroupName(groupName)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );
	setup();
	addReplies(replies);
}

PulseDetails::PulseDetails(PulseHolder *actions,
	RsGxsGroupId &parentGroupId,
	std::string  &parentGroupName,
	RsGxsMessageId &parentOrigMsgId,
	RsGxsId		&parentAuthorId,
	rstime_t	   &parentPublishTs,
	std::string	&parentPulseText)
:QWidget(NULL), mActions(actions), mPulse(), mGroupName(parentGroupName)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );

	// reuse Meta data structure.
	mPulse.mMeta.mGroupId = parentGroupId;
	mPulse.mMeta.mOrigMsgId = parentOrigMsgId;
	mPulse.mMeta.mAuthorId  = parentAuthorId;
	mPulse.mMeta.mPublishTs = parentPublishTs;
	mPulse.mPulseText = parentPulseText;
	setup();
}

void PulseDetails::setBackground(QString color)
{
	QWidget *tocolor = this;
	QPalette p = tocolor->palette();
	p.setColor(tocolor->backgroundRole(), QColor(color));
	tocolor->setPalette(p);
	tocolor->setAutoFillBackground(true);
}

void PulseDetails::setup()
{
	connect(toolButton_expand, SIGNAL(clicked()), this, SLOT(toggle()));

	connect(toolButton_follow, SIGNAL(clicked()), this, SLOT(follow()));
	connect(toolButton_rate, SIGNAL(clicked()), this, SLOT(rate()));
	connect(toolButton_reply, SIGNAL(clicked()), this, SLOT(reply()));

	label_wireName->setText(QString::fromStdString(mGroupName));
	label_idName->setId(mPulse.mMeta.mAuthorId);

	label_date->setText(DateTime::formatDateTime(mPulse.mMeta.mPublishTs));
	label_summary->setText(getSummary());

	// label_icon->setText();
	textBrowser->setPlainText(QString::fromStdString(mPulse.mPulseText));
	frame_expand->setVisible(false);

	label_replies->setText("");
	frame_replies->setVisible(false);
	mHasReplies = false;

	toolButton_follow->setEnabled(mActions != NULL);
	toolButton_rate->setEnabled(mActions != NULL);
	toolButton_reply->setEnabled(mActions != NULL);
}

void PulseDetails::addReplies(std::map<rstime_t, RsWirePulse *> replies)
{
	if (replies.size() == 0)
	{
		// do nothing.
		return;
	}
	else if (replies.size() == 1)
	{
		label_replies->setText("1 reply");
	}
	else if (replies.size() > 1)
	{
		label_replies->setText(QString("%1 replies").arg(replies.size()));
	}

	// add extra widgets into layout.
	QLayout *vbox = frame_replies->layout();
	mHasReplies = true;

	std::map<rstime_t, RsWirePulse *> emptyReplies;
	std::map<rstime_t, RsWirePulse *>::reverse_iterator it;
	for (it = replies.rbegin(); it != replies.rend(); it++)
	{
		// add Ref as child reply.
		PulseDetails *pd = new PulseDetails(mActions,
            it->second->mRefGroupId,
            it->second->mRefGroupName,
            it->second->mRefOrigMsgId,
            it->second->mRefAuthorId,
            it->second->mRefPublishTs,
            it->second->mRefPulseText);
		pd->setBackground("goldenrod");
		vbox->addWidget(pd);
	}
}


void PulseDetails::toggle()
{
	if (frame_expand->isVisible()) {
		// switch to minimal view.
		label_summary->setVisible(true);
		frame_expand->setVisible(false);
		frame_replies->setVisible(false);
	} else {
		// switch to expanded view.
		label_summary->setVisible(false);
		frame_expand->setVisible(true);
		if (mHasReplies) {
			frame_replies->setVisible(true);
		}
	}
}

QString PulseDetails::getSummary()
{
	std::string summary = mPulse.mPulseText;
	std::cerr << "PulseDetails::getSummary() orig: " << summary;
	std::cerr << std::endl;
	int len = summary.size();
	bool in_whitespace = false;
	for (int i = 0; i < len; i++)
	{
		if (isspace(summary[i])) {
			if (in_whitespace) {
				// trim
				summary.erase(i, 1);
				// rollback index / len.
				--i;
				--len;
			} else {
				// replace whitespace with space.
				summary[i] = ' ';
				in_whitespace = true;
			}
		} else {
			in_whitespace = false;
		}
	}
	std::cerr << "PulseDetails::getSummary() summary: " << summary;
	std::cerr << std::endl;

	return QString::fromStdString(summary);
}

void PulseDetails::follow()
{
	// follow group.
	if (mActions)
	{
		mActions->follow(mPulse.mMeta.mGroupId);
	}
}

void PulseDetails::rate()
{
	// rate author
	if (mActions)
	{
		mActions->rate(mPulse.mMeta.mAuthorId);
	}
}

void PulseDetails::reply()
{
	// reply
	if (mActions)
	{
		mActions->reply(mPulse, mGroupName);
	}
}
		

