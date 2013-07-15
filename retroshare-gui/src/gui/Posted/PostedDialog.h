/*
 * Retroshare Posted Dialog
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

#ifndef MRK_POSTED_DIALOG_H
#define MRK_POSTED_DIALOG_H

#include <retroshare/rsposted.h>

#include "gui/gxs/GxsCommentContainer.h"
#include "gui/Posted/PostedListDialog.h"
#include "gui/Posted/PostedItem.h"

class PostedDialog : public GxsCommentContainer
{
	Q_OBJECT

public:
	PostedDialog(QWidget *parent = 0)
	:GxsCommentContainer(parent) { return; }

	virtual GxsServiceDialog *createServiceDialog()
	{
		return new PostedListDialog(this);
	}

	virtual QString getServiceName()
	{
		return tr("Posted Links");
	}

	virtual RsTokenService *getTokenService()
	{
		return rsPosted->getTokenService();
	}

	virtual RsGxsCommentService *getCommentService()
	{
		return rsPosted;
	}

	virtual QWidget *createHeaderWidget(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId)
	{
		return new PostedItem(NULL, 0, grpId, msgId, true);
	}

	virtual QPixmap getServicePixmap()
	{
		return QPixmap(":/images/posted_24.png");
	}
};

#endif
