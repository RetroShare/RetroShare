/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/BoardPostsWidget.h                       *
 *                                                                             *
 * Copyright 2013 by Robert Fernie     <retroshare.project@gmail.com>          *
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

#ifndef _GXS_CHANNELPOSTSWIDGET_H
#define _GXS_CHANNELPOSTSWIDGET_H

#include <map>

#include <QStyledItemDelegate>

#include "retroshare/rsposted.h"

#include "gui/gxs/GxsMessageFramePostWidget.h"
#include "gui/feeds/FeedHolder.h"

namespace Ui {
class PostedListWidgetWithModel;
}

class QTreeWidgetItem;
class QSortFilterProxyModel;
class RsPostedPostsModel;

class PostedPostDelegate: public QAbstractItemDelegate
{
	Q_OBJECT

	public:
		PostedPostDelegate(QObject *parent=0) : QAbstractItemDelegate(parent),mCellWidthPix(100){}
        virtual ~PostedPostDelegate(){}

		void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override;
    	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        int cellSize(const QFont& font) const;

        void setCellWidth(int pix) { mCellWidthPix = pix; }

	private:
        int mCellWidthPix;
};

class PostedListWidgetWithModel: public GxsMessageFrameWidget
{
	Q_OBJECT

public:
	/* Filters */
	enum Filter {
		SORT_TITLE =     1,
		SORT_MSG =       2,
		SORT_FILE_NAME = 3
	};

public:
	/** Default Constructor */
	PostedListWidgetWithModel(const RsGxsGroupId &postedId, QWidget *parent = 0);
	/** Default Destructor */
	~PostedListWidgetWithModel();

	/* GxsMessageFrameWidget */
	virtual QIcon groupIcon() override;
    virtual void groupIdChanged() override { updateDisplay(true); }
    virtual QString groupName(bool) override ;
    virtual bool navigate(const RsGxsMessageId&) override;

	void updateDisplay(bool complete) ;

#ifdef TODO
	/* FeedHolder */
	virtual QScrollArea *getScrollArea();
	virtual void deleteFeedItem(FeedItem *feedItem, uint32_t type);
	virtual void openChat(const RsPeerId& peerId);
#endif
	virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId> &msg_versions, const RsGxsMessageId &msgId, const QString &title);

protected:
	/* GxsMessageFramePostWidget */
	virtual void groupNameChanged(const QString &name);

#ifdef TODO
	virtual bool insertGroupData(const RsGxsGenericGroupData *data) override;
#endif
    virtual void blank() ;

#ifdef TODO
	virtual bool getGroupData(RsGxsGenericGroupData *& data) override;
    virtual void getMsgData(const std::set<RsGxsMessageId>& msgIds,std::vector<RsGxsGenericMsgData*>& posts) override;
    virtual void getAllMsgData(std::vector<RsGxsGenericMsgData*>& posts) override;
	virtual void insertPosts(const std::vector<RsGxsGenericMsgData*>& posts) override;
	virtual void insertAllPosts(const std::vector<RsGxsGenericMsgData*>& posts, GxsMessageFramePostThread *thread) override;
#endif

	/* GxsMessageFrameWidget */
	virtual void setAllMessagesReadDo(bool read, uint32_t &token) override;

private slots:
	void updateGroupData();
	void createMsg();
	void subscribeGroup(bool subscribe);
	void setViewMode(int viewMode);
	void settingsChanged();
	void postChannelPostLoad();
	void postContextMenu(const QPoint&);
	void copyMessageLink();

public slots:
	void handlePostsTreeSizeChange(QSize size);

private:
	void processSettings(bool load);
	int viewMode();

	void insertBoardDetails(const RsPostedGroup &group);
	void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

private:
    RsPostedGroup mGroup;
    RsEventsHandlerId_t mEventHandlerId ;

    RsPostedPostsModel *mPostedPostsModel;
    PostedPostDelegate *mPostedPostsDelegate;

    RsGxsMessageId mSelectedPost;

	/* UI - from Designer */
	Ui::PostedListWidgetWithModel *ui;
};

#endif
