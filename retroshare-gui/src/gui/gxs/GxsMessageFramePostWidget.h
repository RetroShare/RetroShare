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

#ifndef GXSMESSAGEFRAMEPOSTWIDGET_H
#define GXSMESSAGEFRAMEPOSTWIDGET_H

#include "GxsMessageFrameWidget.h"
#include "util/TokenQueue.h"

class GxsFeedItem;
class UIStateHelper;

class GxsMessageFramePostWidget : public GxsMessageFrameWidget, public TokenResponse
{
	Q_OBJECT

public:
	explicit GxsMessageFramePostWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent = NULL);

	/* GxsMessageFrameWidget */
	virtual RsGxsGroupId groupId();
	virtual void setGroupId(const RsGxsGroupId &groupId);
	virtual QString groupName(bool withUnreadCount);
//	virtual QIcon groupIcon() = 0;
	virtual void setAllMessagesRead(bool read);

	/* GXS functions */
	uint32_t nextTokenType() { return ++mNextTokenType; }
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

	int subscribeFlags() { return mSubscribeFlags; }

protected:
	virtual void updateDisplay(bool complete);
	virtual void groupNameChanged(const QString &/*name*/) {}

	virtual void setMessageRead(GxsFeedItem *item, bool read) = 0;
	virtual void clearPosts();

	/* GXS functions */
	void requestGroupData();
	void loadGroupData(const uint32_t &token);
	virtual bool insertGroupData(const uint32_t &token, RsGroupMetaData &metaData) = 0;

	void requestPosts();
	void loadPosts(const uint32_t &token);
	virtual void insertPosts(const uint32_t &token) = 0;

	void requestRelatedPosts(const std::vector<RsGxsMessageId> &msgIds);
	void loadRelatedPosts(const uint32_t &token);
	virtual void insertRelatedPosts(const uint32_t &token) = 0;

protected:
	TokenQueue *mTokenQueue;
	uint32_t mTokenTypeGroupData;
	uint32_t mTokenTypePosts;
	uint32_t mTokenTypeRelatedPosts;
	UIStateHelper *mStateHelper;
	QList<GxsFeedItem*> mPostItems;

private:
	RsGxsGroupId mGroupId; /* current group */
	QString mGroupName;
	int mSubscribeFlags;
	uint32_t mNextTokenType;
};

#endif // GXSMESSAGEFRAMEPOSTWIDGET_H
