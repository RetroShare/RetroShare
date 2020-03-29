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
PulseDetails::PulseDetails(PulseHolder *actions, RsWirePulse &pulse, std::string &groupName, bool is_original)
:QWidget(NULL), mActions(actions), mPulse(pulse), mGroupName(groupName), mIsOriginal(is_original)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );
	setup();
}

PulseDetails::PulseDetails(PulseHolder *actions,
	RsGxsGroupId &parentGroupId,
	std::string  &parentGroupName,
	RsGxsMessageId &parentOrigMsgId,
	RsGxsId		&parentAuthorId,
	rstime_t	   &parentPublishTs,
	std::string	&parentPulseText)
:QWidget(NULL), mActions(actions), mPulse(), mGroupName(parentGroupName), mIsOriginal(false)
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
}


void PulseDetails::toggle()
{
	if (frame_expand->isVisible()) {
		// switch to minimal view.
		label_summary->setVisible(true);
		frame_expand->setVisible(false);
	} else {
		// switch to expanded view.
		label_summary->setVisible(false);
		frame_expand->setVisible(true);
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
	mActions->follow(mPulse.mMeta.mGroupId);
}

void PulseDetails::rate()
{
	// rate author
	mActions->rate(mPulse.mMeta.mAuthorId);
}

void PulseDetails::reply()
{
	mActions->reply(mPulse, mGroupName);
}
		

