/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsGroupFrameDialog.h                            *
 *                                                                             *
 * Copyright 2012-2013  by Robert Fernie      <retroshare.project@gmail.com>   *
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

#ifndef _GXSGROUPFRAMEDIALOG_H
#define _GXSGROUPFRAMEDIALOG_H

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "util/RsUserdata.h"
#include "GxsStatisticsProvider.h"

#include <inttypes.h>

#include "GxsIdTreeWidgetItem.h"
#include "GxsGroupDialog.h"

namespace Ui {
class GxsGroupFrameDialog;
}

class GroupTreeWidget;
class GroupItemInfo;
class GxsMessageFrameWidget;
class UIStateHelper;
struct RsGxsCommentService;
class GxsCommentDialog;

class GxsGroupFrameDialog : public GxsStatisticsProvider
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
		ICON_SEARCH,
		ICON_DEFAULT
	};

public:
    GxsGroupFrameDialog(RsGxsIfaceHelper *ifaceImpl, const QString& settings_name,QWidget *parent = 0,bool allow_dist_sync=false);
	virtual ~GxsGroupFrameDialog();

    virtual bool navigate(const RsGxsGroupId &groupId, const RsGxsMessageId& msgId) override;

	virtual QString getHelpString() const =0;

	virtual void getGroupList(std::map<RsGxsGroupId,RsGroupMetaData> &groups) ;

    void getServiceStatistics(GxsServiceStatistic& stats) const ;

    static uint32_t checkDelay(uint32_t time_in_secs);

protected:
	virtual void showEvent(QShowEvent *event) override;
	virtual void paintEvent(QPaintEvent *pe) override;
	virtual void updateDisplay(bool complete);

	const RsGxsGroupId &groupId() { return mGroupId; }
	void setSingleTab(bool singleTab);
	void setHideTabBarWithOneTab(bool hideTabBarWithOneTab);
	bool getCurrentGroupName(QString& name);
	virtual RetroShareLink::enumType getLinkType() = 0;
	virtual GroupFrameSettings::Type groupFrameSettingsType() { return GroupFrameSettings::Nothing; }
	virtual void groupInfoToGroupItemInfo(const RsGxsGenericGroupData *groupInfo, GroupItemInfo &groupItemInfo);
    virtual void checkRequestGroup(const RsGxsGroupId& /* grpId */) {}	// overload this one in order to retrieve full group data when the group is browsed

	void updateMessageSummaryList(RsGxsGroupId groupId);

    virtual const std::set<TurtleRequestId> getSearchRequests() const { return std::set<TurtleRequestId>(); } // overload this for subclasses that provide distant search

    // This needs to be overloaded by subsclasses, possibly calling the blocking API, since it is used asynchroneously.
    virtual bool getGroupData(std::list<RsGxsGenericGroupData*>& groupInfo) =0;

    void updateMessageSummaryListReal(RsGxsGroupId groupId);

private slots:
	void todo();

	/** Create the context popup menu and it's submenus */
	void groupTreeCustomPopupMenu(QPoint point);
	void settingsChanged();
    void setSyncPostsDelay();
    void setStorePostsDelay();

	void restoreGroupKeys();
	void newGroup();
    void distantRequestGroupData();

	void changedCurrentGroup(const QString &groupId);
	void groupTreeMiddleButtonClicked(QTreeWidgetItem *item);
	void openInNewTab();
	void messageTabCloseRequested(int index);
	void messageTabChanged(int index);
	void messageTabInfoChanged(QWidget *widget);
	void messageTabWaitingChanged(QWidget *widget);

	void copyGroupLink();

	void subscribeGroup();
	void unsubscribeGroup();

	void showGroupDetails();
	void editGroupDetails();

	void markMsgAsRead();
	void markMsgAsUnread();

	void sharePublishKey();

	void loadComment(const RsGxsGroupId &grpId, const QVector<RsGxsMessageId>& msg_versions,const RsGxsMessageId &most_recent_msgId, const QString &title);

    void searchNetwork(const QString &search_string) ;
	void removeAllSearches();
	void removeCurrentSearch();

private:
	virtual QString text(TextType type) = 0;
	virtual QString icon(IconType type) = 0;
    virtual TurtleRequestId distantSearch(const QString& search_string) ;

	virtual GxsGroupDialog *createNewGroupDialog() = 0;
	virtual GxsGroupDialog *createGroupDialog(GxsGroupDialog::Mode mode, RsGxsGroupId groupId) = 0;
	virtual int shareKeyType() = 0;
	virtual GxsMessageFrameWidget *createMessageFrameWidget(const RsGxsGroupId &groupId) = 0;
	virtual void groupTreeCustomActions(RsGxsGroupId /*grpId*/, int /*subscribeFlags*/, QList<QAction*> &/*actions*/) {}
	virtual RsGxsCommentService *getCommentService() { return NULL; }
	virtual QWidget *createCommentHeaderWidget(const RsGxsGroupId &/*grpId*/, const RsGxsMessageId &/*msgId*/) { return NULL; }
    virtual bool getDistantSearchResults(TurtleRequestId /* id */, std::map<RsGxsGroupId,RsGxsGroupSearchResults>& /* group_infos */){ return false ;}
    virtual void clearDistantSearchResults(TurtleRequestId /* id */) {}
    virtual RsGxsGenericGroupData *getDistantSearchResultGroupData(const RsGxsGroupId& /*group_id*/){ return nullptr ;}

	void initUi();

	void openGroupInNewTab(const RsGxsGroupId &groupId);
	void groupSubscribe(bool subscribe);

	void processSettings(bool load);

	// New Request/Response Loading Functions.
	void insertGroupsData(const std::list<RsGxsGenericGroupData *> &groupList);

	//void requestGroupSummary();
	void updateGroupSummary();
	void loadGroupSummary(const std::list<RsGxsGenericGroupData *> &groupInfo);

	virtual uint32_t requestGroupSummaryType() { return GXS_REQUEST_TYPE_GROUP_META; } // request only meta data

	// subscribe/unsubscribe ack.

    virtual GxsMessageFrameWidget *messageWidget(const RsGxsGroupId &groupId) override;
	GxsMessageFrameWidget *createMessageWidget(const RsGxsGroupId &groupId);

	GxsCommentDialog *commentWidget(const RsGxsMessageId &msgId);

protected:

	void updateSearchResults(const TurtleRequestId &sid);
	void updateSearchResults();	// update all searches
    virtual void updateGroupStatistics(const RsGxsGroupId &groupId) override;
    virtual void updateGroupStatisticsReal(const RsGxsGroupId &groupId) override;

private:
	GxsMessageFrameWidget *currentWidget() const;
	bool useTabs();

	bool mInitialized;
	bool mInFill;

	RsGxsGroupId mGroupId;

	QTreeWidgetItem *mYourGroups;
	QTreeWidgetItem *mSubscribedGroups;
	QTreeWidgetItem *mPopularGroups;
	QTreeWidgetItem *mOtherGroups;

    // Message summary list update

	bool mShouldUpdateMessageSummaryList ; // whether we should update the counting for groups. This takes some CPU so we only do it when needed.
    std::set<RsGxsGroupId> mGroupIdsSummaryToUpdate;

    // GroupStatistics update
    rstime_t mLastGroupStatisticsUpdateTs;

	/** Qt Designer generated object */
	Ui::GxsGroupFrameDialog *ui;

	std::map<RsGxsGroupId,RsGroupMetaData> mCachedGroupMetas;

    std::map<uint32_t,QTreeWidgetItem*> mSearchGroupsItems ;
    std::map<uint32_t,std::set<RsGxsGroupId> > mKnownGroups;
};

#endif
