/*
 * Retroshare Channel Dialog
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef MRK_CHANNEL_DIALOG_H
#define MRK_CHANNEL_DIALOG_H

#include <retroshare/rsgxschannels.h>

#include "gui/gxs/GxsCommentContainer.h"
#include "gui/gxschannels/GxsChannelDialog.h"
#include "gui/feeds/GxsChannelPostItem.h"

class ChannelDialog : public GxsCommentContainer
{
	Q_OBJECT

public:
	ChannelDialog(QWidget *parent = 0)
	:GxsCommentContainer(parent) { return; }

	virtual GxsServiceDialog *createServiceDialog()
	{
		return new GxsChannelDialog(this);
	}

	virtual QString getServiceName()
	{
		return tr("GxsChannels");
	}

	virtual RsTokenService *getTokenService()
	{
		return rsGxsChannels->getTokenService();
	}

	virtual RsGxsCommentService *getCommentService()
	{
		return rsGxsChannels;
	}

	virtual QWidget *createHeaderWidget(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId)
	{
		return new GxsChannelPostItem(NULL, 0, grpId, msgId, true);
	}

	virtual QPixmap getServicePixmap()
	{
		return QPixmap(":/images/channels24.png");
	}
};

#endif
