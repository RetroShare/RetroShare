/*******************************************************************************
 * gui/TheWire/PulseAddDialog.h                                                *
 *                                                                             *
 * Copyright (c) 2012-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#ifndef MRK_PULSE_ADD_DIALOG_H
#define MRK_PULSE_ADD_DIALOG_H

#include "ui_PulseAddDialog.h"

#include <retroshare/rswire.h>

class PulseAddDialog : public QWidget
{
  Q_OBJECT

public:
	PulseAddDialog(QWidget *parent = 0);

	void cleanup();

	void setReplyTo(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId);
	void setGroup(const RsGxsGroupId &grpId);

private slots:
	void addURL();
	void clearDisplayAs();
	void postPulse();
	void cancelPulse();
	void clearDialog();
	void pulseTextChanged();

private:
	// OLD VERSIONs, private now.
	void setGroup(RsWireGroup &group);
	void setReplyTo(RsWirePulse &pulse, std::string &groupName);

	void postOriginalPulse();
	void postReplyPulse();

	uint32_t toPulseSentiment(int index);

protected:

	RsWireGroup mGroup; // replyWith.

	// if this is a reply
	bool mIsReply;
	RsWirePulse mReplyToPulse;

	Ui::PulseAddDialog ui;

};

#endif

