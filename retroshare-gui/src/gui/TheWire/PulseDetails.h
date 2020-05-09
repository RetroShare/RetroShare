/*******************************************************************************
 * gui/TheWire/PulseDetails.h                                                  *
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

#ifndef MRK_PULSE_DETAILS_H
#define MRK_PULSE_DETAILS_H

#include "ui_PulseDetails.h"
#include "PulseItem.h"

#include <retroshare/rswire.h>

class PulseDetails : public QWidget, private Ui::PulseDetails
{
  Q_OBJECT

public:
	PulseDetails(PulseHolder *actions, RsWirePulse *pulse, std::string &groupName,
		std::map<rstime_t, RsWirePulse *> replies);

	// when Reply parent....
	PulseDetails(PulseHolder *actions,
		RsGxsGroupId   &parentGroupId,
		std::string    &parentGroupName,
		RsGxsMessageId &parentOrigMsgId,
		RsGxsId	       &parentAuthorId,
		rstime_t       &parentPublishTs,
		std::string	&parentPulseText);

	void setup();

	void setBackground(QString color);

private slots:
	void toggle();
	void follow();
	void rate();
	void reply();

private:
	void addReplies(std::map<rstime_t, RsWirePulse *> replies);
	QString getSummary();

	PulseHolder *mActions;
	RsWirePulse  mPulse;
	std::string  mGroupName;
	bool mHasReplies;
};

#endif
