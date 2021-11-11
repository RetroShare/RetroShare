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


QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
QT_END_NAMESPACE


class PulseAddDialog : public QWidget
{
  Q_OBJECT

public:
	PulseAddDialog(QWidget *parent = 0);

	void cleanup();

	void setReplyTo(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId, uint32_t replyType);
	void setGroup(const RsGxsGroupId &grpId);

private slots:
	void addURL();
	void clearDisplayAs();
	void postPulse();
	void cancelPulse();
	void clearDialog();
	void pulseTextChanged();
	void toggle();

private:
	// OLD VERSIONs, private now.
	void setGroup(RsWireGroup &group);
	void setReplyTo(const RsWirePulse &pulse, RsWirePulseSPtr pPulse, std::string &groupName, uint32_t replyType);

	void postOriginalPulse();
	void postReplyPulse();

	uint32_t toPulseSentiment(int index);

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

	void addImage(const QString &path);

	RsWireGroup mGroup; // replyWith.

	// if this is a reply
	bool mIsReply;
	RsWirePulse mReplyToPulse;
	uint32_t mReplyType;

	// images
	RsGxsImage mImage1;
	RsGxsImage mImage2;
	RsGxsImage mImage3;
	RsGxsImage mImage4;

	Ui::PulseAddDialog ui;
};

#endif

