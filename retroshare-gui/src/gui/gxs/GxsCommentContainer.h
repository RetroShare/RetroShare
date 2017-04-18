/*
 * Retroshare Comment Container Dialog
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

#ifndef MRK_COMMENT_CONTAINER_DIALOG_H
#define MRK_COMMENT_CONTAINER_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_GxsCommentContainer.h"

#include <retroshare/rsgxscommon.h>
#include <retroshare/rstokenservice.h>

class GxsServiceDialog;

class GxsCommentContainer : public MainPage
{
	Q_OBJECT

public:
	explicit GxsCommentContainer(QWidget *parent = 0);
	void setup();

	void commentLoad(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId, const QString &title);

	virtual GxsServiceDialog *createServiceDialog() = 0;
	virtual QString getServiceName() = 0;
	virtual RsTokenService *getTokenService() = 0;
	virtual RsGxsCommentService *getCommentService() = 0;
	virtual QWidget *createHeaderWidget(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId) = 0;
	virtual QPixmap getServicePixmap() = 0;

private slots:
	void tabCloseRequested(int index);

private:
	GxsServiceDialog *mServiceDialog;

	/* UI - from Designer */
	Ui::GxsCommentContainer ui;
};

class GxsServiceDialog
{
public:
	explicit GxsServiceDialog(GxsCommentContainer *container)
	:mContainer(container) { return; }

	virtual ~GxsServiceDialog() { return; }

	void commentLoad(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId, const QString &title)
	{
		mContainer->commentLoad(grpId, msgId, title);
	}

private:
	GxsCommentContainer *mContainer;
};

#endif
