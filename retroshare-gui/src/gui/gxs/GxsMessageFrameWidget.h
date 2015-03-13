/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef GXSMESSAGEFRAMEWIDGET_H
#define GXSMESSAGEFRAMEWIDGET_H

#include "gui/gxs/RsGxsUpdateBroadcastWidget.h"
#include "util/TokenQueue.h"

class RsGxsIfaceHelper;
class UIStateHelper;

class GxsMessageFrameWidget : public RsGxsUpdateBroadcastWidget, public TokenResponse
{
	Q_OBJECT

public:
	explicit GxsMessageFrameWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent = NULL);
	virtual ~GxsMessageFrameWidget();

	const RsGxsGroupId &groupId();
	void setGroupId(const RsGxsGroupId &groupId);

	virtual void groupIdChanged() = 0;
	virtual QString groupName(bool withUnreadCount) = 0;
	virtual QIcon groupIcon() = 0;
	virtual void setAllMessagesRead(bool read) = 0;
	virtual bool navigate(const RsGxsMessageId& msgId) = 0;

	/* GXS functions */
	uint32_t nextTokenType() { return ++mNextTokenType; }
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

signals:
	void groupChanged(QWidget *widget);
	void loadComment(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title);

protected:
	TokenQueue *mTokenQueue;
	UIStateHelper *mStateHelper;

private:
	RsGxsGroupId mGroupId; /* current group */
	uint32_t mNextTokenType;
};

#endif // GXSMESSAGEFRAMEWIDGET_H
