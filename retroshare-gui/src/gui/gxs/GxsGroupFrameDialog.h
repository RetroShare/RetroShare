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

#ifndef _GXSGROUPFRAMEDIALOG_H
#define _GXSGROUPFRAMEDIALOG_H

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"
#include "RsAutoUpdatePage.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "util/RsUserdata.h"

#include <inttypes.h>

#include "util/TokenQueue.h"
#include "GxsIdTreeWidgetItem.h"
#include "GxsGroupDialog.h"

namespace Ui {
class GxsGroupFrameDialog;
}

class GroupTreeWidget;
class GroupItemInfo;
class GxsMessageFrameWidget;
class UIStateHelper;
class RsGxsCommentService;
class GxsCommentDialog;

class GxsGroupFrameDialog : public RsGxsUpdateBroadcastPage, public TokenResponse
{
	Q_OBJECT

public:
	enum TextType {
		TEXT_NAME,
		TEXT_NEW,
		TEXT_TODO,
		TEXT_YOUR_GROUP,
		TEXT_SUBSCRIBED_GROUP,
		TEXT_POPULAR_GROUP,
		TEXT_OTHER_GROUP
	};

	enum IconType {
		ICON_NAME,
		ICON_NEW,
		ICON_YOUR_GROUP,
		ICON_SUBSCRIBED_GROUP,
		ICON_POPULAR_GROUP,
		ICON_OTHER_GROUP,
		ICON_DEFAULT
	};

public:
	GxsGroupFrameDialog(RsGxsIfaceHelper *ifaceImpl, QWidget *parent = 0);
	virtual ~GxsGroupFrameDialog();

	bool navigate(const RsGxsGroupId groupId, const RsGxsMessageId& msgId);

	// Callback for all Loads.
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

	virtual QString getHelpString() const =0;

protected:
	virtual void showEvent(QShowEvent *event);
	virtual void updateDisplay(bool complete);

	RsGxsGroupId groupId() { return mGroupId; }
	void setSingleTab(bool singleTab);
	void setHideTabBarWithOneTab(bool hideTabBarWithOneTab);
	bool getCurrentGroupName(QString& name);
	virtual RetroShareLink::enumType getLinkType() = 0;
	virtual GroupFrameSettings::Type groupFrameSettingsType() { return GroupFrameSettings::Nothing; }
	virtual void groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo, const RsUserdata *userdata);

private slots:
	void todo();

	/** Create the context popup menu and it's submenus */
	void groupTreeCustomPopupMenu(QPoint point);
	void settingsChanged();

	void restoreGroupKeys();
	void newGroup();

	void changedGroup(const QString &groupId);
	void groupTreeMiddleButtonClicked(QTreeWidgetItem *item);
	void openInNewTab();
	void messageTabCloseRequested(int index);
	void messageTabChanged(int index);
	void messageTabInfoChanged(QWidget *widget);

	void copyGroupLink();

	void subscribeGroup();
	void unsubscribeGroup();

	void showGroupDetails();
	void editGroupDetails();

	void markMsgAsRead();
	void markMsgAsUnread();

	void shareKey();

	void loadComment(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId, const QString &title);

private:
	virtual QString text(TextType type) = 0;
	virtual QString icon(IconType type) = 0;
	virtual QString settingsGroupName() = 0;

	virtual GxsGroupDialog *createNewGroupDialog(TokenQueue *tokenQueue) = 0;
	virtual GxsGroupDialog *createGroupDialog(TokenQueue *tokenQueue, RsTokenService *tokenService, GxsGroupDialog::Mode mode, RsGxsGroupId groupId) = 0;
	virtual int shareKeyType() = 0;
	virtual GxsMessageFrameWidget *createMessageFrameWidget(const RsGxsGroupId &groupId) = 0;
	virtual void groupTreeCustomActions(RsGxsGroupId /*grpId*/, int /*subscribeFlags*/, QList<QAction*> &/*actions*/) {}
	virtual RsGxsCommentService *getCommentService() { return NULL; }
	virtual QWidget *createCommentHeaderWidget(const RsGxsGroupId &/*grpId*/, const RsGxsMessageId &/*msgId*/) { return NULL; }

	void initUi();

	void updateMessageSummaryList(RsGxsGroupId groupId);

	void openGroupInNewTab(const RsGxsGroupId &groupId);
	void groupSubscribe(bool subscribe);

	void processSettings(bool load);

	// New Request/Response Loading Functions.
	void insertGroupsData(const std::list<RsGroupMetaData> &groupList, const RsUserdata *userdata);

	void requestGroupSummary();
	void loadGroupSummary(const uint32_t &token);
	virtual uint32_t requestGroupSummaryType() { return GXS_REQUEST_TYPE_GROUP_META; } // request only meta data
	virtual void loadGroupSummaryToken(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo, RsUserdata* &userdata); // use with requestGroupSummaryType

	void requestGroupStatistics(const RsGxsGroupId &groupId);
	void loadGroupStatistics(const uint32_t &token);

	// subscribe/unsubscribe ack.
//	void acknowledgeSubscribeChange(const uint32_t &token);

	GxsMessageFrameWidget *messageWidget(const RsGxsGroupId &groupId, bool ownTab);
	GxsMessageFrameWidget *createMessageWidget(const RsGxsGroupId &groupId);

	GxsCommentDialog *commentWidget(const RsGxsMessageId &msgId);

//	void requestGroupSummary_CurrentGroup(const  RsGxsGroupId &groupId);
//	void loadGroupSummary_CurrentGroup(const uint32_t &token);

private:
	bool mInitialized;
	QString mSettingsName;
	RsGxsGroupId mGroupId;
	RsGxsIfaceHelper *mInterface;
	RsTokenService *mTokenService;
	TokenQueue *mTokenQueue;
	GxsMessageFrameWidget *mMessageWidget;

	QTreeWidgetItem *mYourGroups;
	QTreeWidgetItem *mSubscribedGroups;
	QTreeWidgetItem *mPopularGroups;
	QTreeWidgetItem *mOtherGroups;

	UIStateHelper *mStateHelper;

	/** Qt Designer generated object */
	Ui::GxsGroupFrameDialog *ui;
};

#endif
