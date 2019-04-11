/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCommentContainer.h                            *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie   <retroshare.project@gmail.com>       *
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
	GxsCommentContainer(QWidget *parent = 0);
	void setup();

	void commentLoad(const RsGxsGroupId &grpId, const std::set<RsGxsMessageId> &msg_versions, const RsGxsMessageId &msgId, const QString &title);

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
	GxsServiceDialog(GxsCommentContainer *container)
	:mContainer(container) { return; }

	virtual ~GxsServiceDialog() { return; }

	void commentLoad(const RsGxsGroupId &grpId, const std::set<RsGxsMessageId>& msg_versions,const RsGxsMessageId &msgId, const QString &title)
	{
		mContainer->commentLoad(grpId, msg_versions,msgId, title);
	}

private:
	GxsCommentContainer *mContainer;
};

#endif
