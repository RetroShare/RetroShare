/*******************************************************************************
 * gui/TheWire/PulseReply.h                                                    *
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

#ifndef MRK_PULSE_REPLY_H
#define MRK_PULSE_REPLY_H

#include "ui_PulseReply.h"
#include "PulseViewItem.h"

#include <retroshare/rswire.h>

class PulseReply : public PulseDataItem, private Ui::PulseReply
{
  Q_OBJECT

public:
	PulseReply(PulseViewHolder *holder, RsWirePulseSPtr pulse);

	void showReplyLine(bool enable);

protected:
	void setup();

// PulseDataInterface ===========
	// Group
	virtual void setHeadshot(const QPixmap &pixmap) override;
	virtual void setGroupNameString(QString name) override;
	virtual void setAuthorString(QString name) override;

	// Msg
	virtual void setRefMessage(QString msg, uint32_t image_count) override;
	virtual void setMessage(RsWirePulseSPtr pulse) override;
	virtual void setDateString(QString date) override;

	// Refs
	virtual void setLikesString(QString likes) override;
	virtual void setRepublishesString(QString repub) override;
	virtual void setRepliesString(QString reply) override;

	// 
	virtual void setReferenceString(QString ref) override;
	virtual void setPulseStatus(PulseStatus status) override;
// PulseDataInterface ===========

	void mousePressEvent(QMouseEvent *event);
};

#endif
