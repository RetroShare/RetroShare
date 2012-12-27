/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#ifndef _GXSFORUMSDIALOG_H
#define _GXSFORUMSDIALOG_H

#include <QThread>

#include "mainpage.h"
#include "RsAutoUpdatePage.h"
#include "ui_GxsForumsDialog.h"

#include <inttypes.h>

#include "util/TokenQueue.h"

#include "gui/gxs/GxsIdTreeWidgetItem.h"

class ForumInfo;
class RsGxsForumMsg;
class GxsForumThreadWidget;

class GxsForumsDialog : public RsAutoUpdatePage, public TokenResponse 
{
	Q_OBJECT

public:
	GxsForumsDialog(QWidget *parent = 0);
	~GxsForumsDialog();

//	virtual UserNotify *getUserNotify(QObject *parent);

	bool navigate(const std::string& forumId, const std::string& msgId);

	/* overloaded from RsAuthUpdatePage */
	virtual void updateDisplay();

	// Callback for all Loads.
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
	void forceUpdateDisplay(); // TEMP HACK FN.

	/** Create the context popup menu and it's submenus */
	void forumListCustomPopupMenu( QPoint point );

	void restoreForumKeys();
	void newforum();

	void changedForum(const QString &id);
	void threadTabCloseRequested(int index);
	void threadTabChanged(QWidget *widget);

	void copyForumLink();

	void subscribeToForum();
	void unsubscribeToForum();

	void showForumDetails();
	void editForumDetails();

	void markMsgAsRead();
	void markMsgAsUnread();

	void generateMassData();

	void shareKey();

private:
	void insertForums();
	
	void updateMessageSummaryList(std::string forumId);
//	void forumInfoToGroupItemInfo(const ForumInfo &forumInfo, GroupItemInfo &groupItemInfo);
	void forumInfoToGroupItemInfo(const RsGroupMetaData &forumInfo, GroupItemInfo &groupItemInfo);

	void forumSubscribe(bool subscribe);

	void processSettings(bool load);

	// New Request/Response Loading Functions.
	void insertForumsData(const std::list<RsGroupMetaData> &forumList);

	void requestGroupSummary();
	void loadGroupSummary(const uint32_t &token);

	GxsForumThreadWidget *forumThreadWidget(const std::string &id);

//	void requestGroupSummary_CurrentForum(const std::string &forumId);
//	void loadGroupSummary_CurrentForum(const uint32_t &token);

	std::string mForumId;
	TokenQueue *mForumQueue;

	QTreeWidgetItem *yourForums;
	QTreeWidgetItem *subscribedForums;
	QTreeWidgetItem *popularForums;
	QTreeWidgetItem *otherForums;

	/** Qt Designer generated object */
	Ui::GxsForumsDialog ui;
};

#endif
