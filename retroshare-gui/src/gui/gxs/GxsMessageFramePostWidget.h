/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsMessageFramePostWidget.h                      *
 *                                                                             *
 * Copyright 2014 Retroshare Team           <retroshare.project@gmail.com>     *
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

#ifndef GXSMESSAGEFRAMEPOSTWIDGET_H
#define GXSMESSAGEFRAMEPOSTWIDGET_H

#include <QThread>

#include "GxsMessageFrameWidget.h"

class GxsMessageFramePostThread;

class GxsMessageFramePostWidget : public GxsMessageFrameWidget
{
	Q_OBJECT

	friend class GxsMessageFramePostThread;

public:
	explicit GxsMessageFramePostWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent = NULL);
	virtual ~GxsMessageFramePostWidget();

	/* GxsMessageFrameWidget */
	virtual void groupIdChanged();
	virtual QString groupName(bool withUnreadCount);
	virtual bool navigate(const RsGxsMessageId& msgId);
	virtual bool isLoading();

    // These should be derived in subclasses
	virtual bool getGroupData(RsGxsGenericGroupData *& data) =0;
    virtual void getMsgData(const std::set<RsGxsMessageId>& msgIds,std::vector<RsGxsGenericMsgData*>& posts) =0;
    virtual void getAllMsgData(std::vector<RsGxsGenericMsgData*>& posts) =0;

	/* GXS functions */
//	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

	int subscribeFlags() { return mSubscribeFlags; }

protected:
	/* RsGxsUpdateBroadcastWidget */
	virtual void updateDisplay(bool complete);

	virtual void groupNameChanged(const QString &/*name*/) {}

	virtual void clearPosts() = 0;
	virtual void blank() = 0;
	virtual bool navigatePostItem(const RsGxsMessageId& msgId) = 0;

	/* Thread functions */
	virtual bool useThread() { return false; }
	virtual void fillThreadCreatePost(const QVariant &/*post*/, bool /*related*/, int /*current*/, int /*count*/) {}

	/* GXS functions */
	void requestGroupData();
	void loadGroupData();
	void loadAllPosts();
	void loadPosts(const std::set<RsGxsMessageId>& msgIds);

#ifdef TO_REMOVE
	void requestAllPosts();
	void loadAllPosts();
	virtual void insertAllPosts(const uint32_t &token, GxsMessageFramePostThread *thread) = 0;

	//void requestPosts(const std::set<RsGxsMessageId> &msgIds);
	//void loadPosts(const uint32_t &token);
#endif
	virtual bool insertGroupData(const RsGxsGenericGroupData *data) =0;
	virtual void insertPosts(const std::vector<RsGxsGenericMsgData*>& posts) =0;
	virtual void insertAllPosts(const std::vector<RsGxsGenericMsgData*>& posts, GxsMessageFramePostThread *thread) =0;

private slots:
	void fillThreadFinished();
	void fillThreadAddPost(const QVariant &post, bool related, int current, int count);

protected:
	uint32_t mTokenTypeGroupData;
	uint32_t mTokenTypeAllPosts;
	uint32_t mTokenTypePosts;
	RsGxsMessageId mNavigatePendingMsgId;

private:
	QString mGroupName;
	int mSubscribeFlags;
	GxsMessageFramePostThread *mFillThread;
};

class GxsMessageFramePostThread : public QThread
{
	Q_OBJECT

public:
	GxsMessageFramePostThread(const std::vector<RsGxsGenericMsgData*>& posts,GxsMessageFramePostWidget *parent);
	~GxsMessageFramePostThread();

	void run();
	void stop(bool waitForStop);
	bool stopped() { return mStopped; }

	void emitAddPost(const QVariant &post, bool related, int current, int count);

signals:
	void addPost(const QVariant &post, bool related, int current, int count);

private:
    std::vector<RsGxsGenericMsgData*> mPosts;
	GxsMessageFramePostWidget *mParent;
	volatile bool mStopped;
};

#endif // GXSMESSAGEFRAMEPOSTWIDGET_H
