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
#include "util/TokenQueue.h"

class PulseAddDialog : public QWidget, public TokenResponse
{
  Q_OBJECT

public:
	PulseAddDialog(QWidget *parent = 0);

	void cleanup();
	void setGroup(RsWireGroup &group);
	void setReplyTo(RsWirePulse &pulse, std::string &groupName);

private slots:
	void addURL();
	void clearDisplayAs();
	void postPulse();
	void cancelPulse();
	void clearDialog();
	void pulseTextChanged();

private:
	void postOriginalPulse();
	void postReplyPulse();
	void postRefPulse(RsWirePulse &pulse);

	void acknowledgeMessage(const uint32_t &token);
	void loadPulseData(const uint32_t &token);
	void loadRequest(const TokenQueue *queue, const TokenRequest &req);
	uint32_t toPulseSentiment(int index);

protected:

	RsWireGroup mGroup; // where we want to post from.

	// if this is a reply
	bool mIsReply;
	std::string mReplyGroupName;
	RsWirePulse mReplyToPulse;
	bool mWaitingRefMsg;

	TokenQueue* mWireQueue;
	Ui::PulseAddDialog ui;

};

#endif

