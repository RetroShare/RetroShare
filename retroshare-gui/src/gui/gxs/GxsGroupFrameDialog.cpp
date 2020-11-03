/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsGroupFrameDialog.cpp                          *
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

#include <QMenu>
#include <QMessageBox>
#include <QToolButton>

#include "GxsGroupFrameDialog.h"
#include "ui_GxsGroupFrameDialog.h"
#include "GxsMessageFrameWidget.h"

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsGroupShareKey.h"
#include "gui/common/GroupTreeWidget.h"
#include "gui/common/RSTreeWidget.h"
#include "gui/notifyqt.h"
#include "gui/common/UIStateHelper.h"
#include "gui/common/UserNotify.h"
#include "util/qtthreadsutils.h"
#include "retroshare/rsgxsifacetypes.h"
#include "GxsCommentDialog.h"

//#define DEBUG_GROUPFRAMEDIALOG

/* Images for TreeWidget */
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
//#define IMAGE_GROUPAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"
#define IMAGE_EDIT           ":/icons/png/pencil-edit-button.png"
#define IMAGE_SHARE          ":/images/share-icon-16.png"
#define IMAGE_TABNEW         ":/images/tab-new.png"
#define IMAGE_DELETE         ":/images/delete.png"
#define IMAGE_RETRIEVE       ":/images/edit_add24.png"
#define IMAGE_COMMENT        ""

#define TOKEN_TYPE_GROUP_SUMMARY    1
//#define TOKEN_TYPE_SUBSCRIBE_CHANGE 2
//#define TOKEN_TYPE_CURRENTGROUP     3
#define TOKEN_TYPE_STATISTICS       4

#define MAX_COMMENT_TITLE 32

static const uint32_t DELAY_BETWEEN_GROUP_STATISTICS_UPDATE = 120; // do not update group statistics more often than once every 2 mins

/*
 * Transformation Notes:
 *   there are still a couple of things that the new groups differ from Old version.
 *   these will need to be addressed in the future.
 *     -> Child TS (for sorting) is not handled by GXS, this will probably have to be done in the GUI.
 *     -> Need to handle IDs properly.
 *     -> Much more to do.
 */

/** Constructor */
GxsGroupFrameDialog::GxsGroupFrameDialog(RsGxsIfaceHelper *ifaceImpl, QWidget *parent,bool allow_dist_sync)
: MainPage(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::GxsGroupFrameDialog();
	ui->setupUi(this);

	mShouldUpdateMessageSummaryList = true;
	mShouldUpdateGroupStatistics = false;
    mLastGroupStatisticsUpdateTs=0;
	mInitialized = false;
	mDistSyncAllowed = allow_dist_sync;
	mInFill = false;
	mCountChildMsgs = false;
	mYourGroups = NULL;
	mSubscribedGroups = NULL;
	mPopularGroups = NULL;
	mOtherGroups = NULL;

	/* Setup Queue */
	mInterface = ifaceImpl;

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	mStateHelper->addWidget(TOKEN_TYPE_GROUP_SUMMARY, ui->loadingLabel, UISTATE_LOADING_VISIBLE);

	connect(ui->groupTreeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(groupTreeCustomPopupMenu(QPoint)));
    connect(ui->groupTreeWidget, SIGNAL(treeCurrentItemChanged(QString)), this, SLOT(changedCurrentGroup(QString)));
	connect(ui->groupTreeWidget->treeWidget(), SIGNAL(signalMouseMiddleButtonClicked(QTreeWidgetItem*)), this, SLOT(groupTreeMiddleButtonClicked(QTreeWidgetItem*)));
	connect(ui->messageTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(messageTabCloseRequested(int)));
	connect(ui->messageTabWidget, SIGNAL(currentChanged(int)), this, SLOT(messageTabChanged(int)));

	connect(ui->todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));

	ui->groupTreeWidget->setDistSearchVisible(allow_dist_sync) ;

    if(allow_dist_sync)
		connect(ui->groupTreeWidget, SIGNAL(distantSearchRequested(const QString&)), this, SLOT(searchNetwork(const QString&)));

	/* Set initial size the splitter */
	ui->splitter->setStretchFactor(0, 0);
	ui->splitter->setStretchFactor(1, 1);

	QList<int> sizes;
	sizes << 300 << width(); // Qt calculates the right sizes
	ui->splitter->setSizes(sizes);

#ifndef UNFINISHED
	ui->todoPushButton->hide();
#endif
}

GxsGroupFrameDialog::~GxsGroupFrameDialog()
{
	// save settings
	processSettings(false);

	delete(ui);
}

void GxsGroupFrameDialog::getGroupList(std::map<RsGxsGroupId, RsGroupMetaData> &group_list)
{
	group_list = mCachedGroupMetas ;

	if(group_list.empty())
		updateGroupSummary();
    else
        std::cerr << "************** Using cached GroupMetaData" << std::endl;
}
void GxsGroupFrameDialog::initUi()
{
	registerHelpButton(ui->helpButton, getHelpString(),pageName()) ;

    ui->titleBarPixmap->setPixmap(FilesDefs::getPixmapFromQtResourcePath(icon(ICON_NAME)));
	ui->titleBarLabel->setText(text(TEXT_NAME));

	/* Initialize group tree */
	QToolButton *newGroupButton = new QToolButton(this);
    newGroupButton->setIcon(FilesDefs::getIconFromQtResourcePath(icon(ICON_NEW)));
	newGroupButton->setToolTip(text(TEXT_NEW));
	connect(newGroupButton, SIGNAL(clicked()), this, SLOT(newGroup()));
	ui->groupTreeWidget->addToolButton(newGroupButton);

	/* Create group tree */
    mYourGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_YOUR_GROUP), FilesDefs::getIconFromQtResourcePath(icon(ICON_YOUR_GROUP)), true);
    mSubscribedGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_SUBSCRIBED_GROUP), FilesDefs::getIconFromQtResourcePath(icon(ICON_SUBSCRIBED_GROUP)), true);
    mPopularGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_POPULAR_GROUP), FilesDefs::getIconFromQtResourcePath(icon(ICON_POPULAR_GROUP)), false);
    mOtherGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_OTHER_GROUP), FilesDefs::getIconFromQtResourcePath(icon(ICON_OTHER_GROUP)), false);

	if (text(TEXT_TODO).isEmpty()) {
		ui->todoPushButton->hide();
	}

	// load settings
	mSettingsName = settingsGroupName();
	processSettings(true);

	if (groupFrameSettingsType() != GroupFrameSettings::Nothing) {
		connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
		settingsChanged();
	}

	mInitialized = true;
}

void GxsGroupFrameDialog::showEvent(QShowEvent *event)
{
	if (!mInitialized )
	{
		/* Problem: virtual methods cannot be used in constructor */

		initUi();
	}

    uint32_t children =  mYourGroups->childCount() + mSubscribedGroups->childCount() + mPopularGroups->childCount() + mOtherGroups->childCount();

    bool empty = mCachedGroupMetas.empty() || children==0;

    updateDisplay( empty );
}

void GxsGroupFrameDialog::paintEvent(QPaintEvent *pe)
{
	if(mShouldUpdateMessageSummaryList)
	{
		if(!mGroupIdsSummaryToUpdate.empty())
			for(auto& group_id: mGroupIdsSummaryToUpdate)
				updateMessageSummaryListReal(group_id);
		else
			updateMessageSummaryListReal(RsGxsGroupId());

		mShouldUpdateMessageSummaryList = false;
		mGroupIdsSummaryToUpdate.clear();
	}

    rstime_t now = time(nullptr);

    if(mShouldUpdateGroupStatistics &&  now > DELAY_BETWEEN_GROUP_STATISTICS_UPDATE + mLastGroupStatisticsUpdateTs)
    {
        // This mechanism allows to gather multiple updateGroupStatistics events at once and not send too many of them at the same time.
        // it avoids re-loadign all the group everytime a friend sends new statistics.

        for(auto& groupId: mGroupStatisticsToUpdate)
			updateGroupStatisticsReal(groupId);

        mShouldUpdateGroupStatistics = false;
        mLastGroupStatisticsUpdateTs = time(nullptr);
        mGroupStatisticsToUpdate.clear();
    }

    MainPage::paintEvent(pe);
}

void GxsGroupFrameDialog::processSettings(bool load)
{
	if (mSettingsName.isEmpty()) {
		return;
	}

	Settings->beginGroup(mSettingsName);

	if (load) {
		// load settings

		// state of splitter
		ui->splitter->restoreState(Settings->value("Splitter").toByteArray());
	} else {
		// save settings

		// state of splitter
		Settings->setValue("Splitter", ui->splitter->saveState());
	}

	ui->groupTreeWidget->processSettings(load);

	Settings->endGroup();
}

bool GxsGroupFrameDialog::useTabs()
{
	GroupFrameSettings groupFrameSettings;

	return Settings->getGroupFrameSettings(groupFrameSettingsType(), groupFrameSettings)  && groupFrameSettings.mOpenAllInNewTab;
}

void GxsGroupFrameDialog::settingsChanged()
{
	GroupFrameSettings groupFrameSettings;
	if (Settings->getGroupFrameSettings(groupFrameSettingsType(), groupFrameSettings)) {
		setSingleTab(!groupFrameSettings.mOpenAllInNewTab);
		setHideTabBarWithOneTab(groupFrameSettings.mHideTabBarWithOneTab);
	}
}

void GxsGroupFrameDialog::setSingleTab(bool singleTab)
{
	if (singleTab)
    {
        while(ui->messageTabWidget->count() > 1)
        {
            auto w = ui->messageTabWidget->widget(0) ;
            ui->messageTabWidget->removeTab(0);
            delete w;
        }
		ui->messageTabWidget->hideCloseButton(0);
	}
}

void GxsGroupFrameDialog::setHideTabBarWithOneTab(bool hideTabBarWithOneTab)
{
	ui->messageTabWidget->setHideTabBarWithOneTab(hideTabBarWithOneTab);
}

void GxsGroupFrameDialog::updateDisplay(bool complete)
{
    if(complete)    // || !getGrpIds().empty() || !getGrpIdsMeta().empty()) {
		updateGroupSummary(); /* Update group list */

//    updateSearchResults() ;
}

void GxsGroupFrameDialog::updateSearchResults()
{
    for(auto& it:mSearchGroupsItems)
        updateSearchResults(it.first);
}

void GxsGroupFrameDialog::updateSearchResults(const TurtleRequestId& sid)
{
	std::cerr << "updating search ID " << std::hex << sid << std::dec << std::endl;

	std::map<RsGxsGroupId,RsGxsGroupSearchResults> group_infos;

	getDistantSearchResults(sid,group_infos) ;

	std::cerr << "retrieved " << std::endl;

	auto it2 = mSearchGroupsItems.find(sid);

    if(it2 == mSearchGroupsItems.end())
    {
        std::cerr << "(EE) received a channel group turtle search result with ID " << sid << " but no item is known for this search" << std::endl;
        return;
    }

	QList<GroupItemInfo> group_items ;

	for(auto it3(group_infos.begin());it3!=group_infos.end();++it3)
	{
		std::cerr << "  adding group " << it3->first << " " << it3->second.mGroupId << " \"" << it3->second.mGroupName << "\"" << std::endl;
        for(auto s:it3->second.mSearchContexts)
			std::cerr << "    Context string \"" << s << "\"" << std::endl;

		GroupItemInfo i;
		i.id             = QString(it3->second.mGroupId.toStdString().c_str());
		i.name           = QString::fromUtf8(it3->second.mGroupName.c_str());
		i.popularity     = 0; // could be set to the number of hits
		i.lastpost       = QDateTime::fromTime_t(it3->second.mLastMessageTs);
		i.subscribeFlags = 0; // irrelevant here
		i.publishKey     = false ; // IS_GROUP_PUBLISHER(groupInfo.mSubscribeFlags);
		i.adminKey       = false ; // IS_GROUP_ADMIN(groupInfo.mSubscribeFlags);
		i.max_visible_posts = it3->second.mNumberOfMessages;
		i.context_strings = it3->second.mSearchContexts;

		group_items.push_back(i);
	}

	ui->groupTreeWidget->fillGroupItems(it2->second, group_items);
}

void GxsGroupFrameDialog::todo()
{
	QMessageBox::information(this, "Todo", text(TEXT_TODO));
}

void GxsGroupFrameDialog::removeCurrentSearch()
{
    QAction *action = dynamic_cast<QAction*>(sender()) ;

    if(!action)
        return ;

    TurtleRequestId search_request_id = action->data().toUInt();

    auto it = mSearchGroupsItems.find(search_request_id) ;

    if(it == mSearchGroupsItems.end())
        return ;

    ui->groupTreeWidget->removeSearchItem(it->second) ;
    mSearchGroupsItems.erase(it);

    mKnownGroups.erase(search_request_id);

    clearDistantSearchResults(search_request_id);
}

void GxsGroupFrameDialog::removeAllSearches()
{
    for(auto it(mSearchGroupsItems.begin());it!=mSearchGroupsItems.end();++it)
    {
        QString group_id;
        TurtleRequestId search_request_id;

        if(ui->groupTreeWidget->isSearchRequestResultItem(it->second,group_id,search_request_id))
			clearDistantSearchResults(search_request_id);

		ui->groupTreeWidget->removeSearchItem(it->second) ;
    }
    mSearchGroupsItems.clear();
    mKnownGroups.clear();
}

// Same function than the one in rsgxsnetservice.cc, so that all times are automatically consistent

uint32_t GxsGroupFrameDialog::checkDelay(uint32_t time_in_secs)
{
    if(time_in_secs <    1 * 86400)
        return 0        ;
    if(time_in_secs <=  10 * 86400)
        return 5 * 86400;
    if(time_in_secs <=  20 * 86400)
        return 15 * 86400;
    if(time_in_secs <=  60 * 86400)
        return 30 * 86400;
    if(time_in_secs <= 120 * 86400)
        return 90 * 86400;
    if(time_in_secs <= 250 * 86400)
        return 180 * 86400;

   return 365 * 86400;
}

void GxsGroupFrameDialog::groupTreeCustomPopupMenu(QPoint point)
{
	// First separately handle the case of search top level items

	TurtleRequestId search_request_id = 0 ;

	if(ui->groupTreeWidget->isSearchRequestItem(point,search_request_id))
	{
		QMenu contextMnu(this);

        contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_DELETE), tr("Remove this search"), this, SLOT(removeCurrentSearch()))->setData(search_request_id);
        contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_DELETE), tr("Remove all searches"), this, SLOT(removeAllSearches()));
		contextMnu.exec(QCursor::pos());
		return ;
	}

    // Then check whether we have a searched item, or a normal group

    QString group_id_s ;

	if(ui->groupTreeWidget->isSearchRequestResult(point,group_id_s,search_request_id))
    {
		QMenu contextMnu(this);

        contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_RETRIEVE), tr("Request data"), this, SLOT(distantRequestGroupData()))->setData(group_id_s);
		contextMnu.exec(QCursor::pos());
		return ;
    }

	QString id = ui->groupTreeWidget->itemIdAt(point);
	if (id.isEmpty()) return;

	mGroupId = RsGxsGroupId(id.toStdString());
	int subscribeFlags = ui->groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId.toStdString()));

	bool isAdmin      = IS_GROUP_ADMIN(subscribeFlags);
	bool isPublisher  = IS_GROUP_PUBLISHER(subscribeFlags);
	bool isSubscribed = IS_GROUP_SUBSCRIBED(subscribeFlags);

	QMenu contextMnu(this);
	QAction *action;

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_TABNEW), tr("Open in new tab"), this, SLOT(openInNewTab()));

	if(mGroupId.isNull()) // dont enable the open in tab if a tab is already here
		action->setEnabled(false);

	if (isSubscribed) {
        action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_UNSUBSCRIBE), tr("Unsubscribe"), this, SLOT(unsubscribeGroup()));
		action->setEnabled (!mGroupId.isNull() && IS_GROUP_SUBSCRIBED(subscribeFlags));
	} else {
        action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_SUBSCRIBE), tr("Subscribe"), this, SLOT(subscribeGroup()));
		action->setDisabled (mGroupId.isNull() || IS_GROUP_SUBSCRIBED(subscribeFlags));
	}

	contextMnu.addSeparator();

    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(icon(ICON_NEW)), text(TEXT_NEW), this, SLOT(newGroup()));

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_INFO), tr("Show Details"), this, SLOT(showGroupDetails()));
	action->setEnabled (!mGroupId.isNull());

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_EDIT), tr("Edit Details"), this, SLOT(editGroupDetails()));
	action->setEnabled (!mGroupId.isNull() && isAdmin);

	uint32_t current_store_time = checkDelay(mInterface->getStoragePeriod(mGroupId))/86400 ;
	uint32_t current_sync_time  = checkDelay(mInterface->getSyncPeriod(mGroupId))/86400 ;

	std::cerr << "Got sync=" << current_sync_time << ". store=" << current_store_time << std::endl;
	QAction *actnn = NULL;

	QMenu *ctxMenu2 = contextMnu.addMenu(tr("Synchronise posts of last...")) ;
    actnn = ctxMenu2->addAction(tr(" 5 days"     ),this,SLOT(setSyncPostsDelay())) ; actnn->setData(QVariant(  5)) ; if(current_sync_time ==  5) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 2 weeks"    ),this,SLOT(setSyncPostsDelay())) ; actnn->setData(QVariant( 15)) ; if(current_sync_time == 15) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 1 month"    ),this,SLOT(setSyncPostsDelay())) ; actnn->setData(QVariant( 30)) ; if(current_sync_time == 30) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 3 months"   ),this,SLOT(setSyncPostsDelay())) ; actnn->setData(QVariant( 90)) ; if(current_sync_time == 90) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 6 months"   ),this,SLOT(setSyncPostsDelay())) ; actnn->setData(QVariant(180)) ; if(current_sync_time ==180) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 1 year  "   ),this,SLOT(setSyncPostsDelay())) ; actnn->setData(QVariant(365)) ; if(current_sync_time ==365) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" Indefinitly"),this,SLOT(setSyncPostsDelay())) ; actnn->setData(QVariant(  0)) ; if(current_sync_time ==  0) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    ctxMenu2->setEnabled(isSubscribed);

	ctxMenu2 = contextMnu.addMenu(tr("Store posts for at most...")) ;
    actnn = ctxMenu2->addAction(tr(" 5 days"     ),this,SLOT(setStorePostsDelay())) ; actnn->setData(QVariant(  5)) ; if(current_store_time ==  5) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 2 weeks"    ),this,SLOT(setStorePostsDelay())) ; actnn->setData(QVariant( 15)) ; if(current_store_time == 15) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 1 month"    ),this,SLOT(setStorePostsDelay())) ; actnn->setData(QVariant( 30)) ; if(current_store_time == 30) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 3 months"   ),this,SLOT(setStorePostsDelay())) ; actnn->setData(QVariant( 90)) ; if(current_store_time == 90) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 6 months"   ),this,SLOT(setStorePostsDelay())) ; actnn->setData(QVariant(180)) ; if(current_store_time ==180) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" 1 year  "   ),this,SLOT(setStorePostsDelay())) ; actnn->setData(QVariant(365)) ; if(current_store_time ==365) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    actnn = ctxMenu2->addAction(tr(" Indefinitly"),this,SLOT(setStorePostsDelay())) ; actnn->setData(QVariant(  0)) ; if(current_store_time ==  0) { actnn->setEnabled(false);actnn->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/start.png"));}
    ctxMenu2->setEnabled(isSubscribed);

	if (shareKeyType()) {
        action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_SHARE), tr("Share publish permissions..."), this, SLOT(sharePublishKey()));
		action->setEnabled(!mGroupId.isNull() && isPublisher);
	}

	if (getLinkType() != RetroShareLink::TYPE_UNKNOWN) {
        action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyGroupLink()));
		action->setEnabled(!mGroupId.isNull());
	}

	contextMnu.addSeparator();

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/message-mail-read.png"), tr("Mark all as read"), this, SLOT(markMsgAsRead()));
	action->setEnabled (!mGroupId.isNull() && isSubscribed);

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/message-mail.png"), tr("Mark all as unread"), this, SLOT(markMsgAsUnread()));
	action->setEnabled (!mGroupId.isNull() && isSubscribed);

	/* Add special actions */
	QList<QAction*> actions;
	groupTreeCustomActions(mGroupId, subscribeFlags, actions);
	if (!actions.isEmpty()) {
		contextMnu.addSeparator();
		contextMnu.addActions(actions);
	}

	//Add Standard Menu
	ui->groupTreeWidget->treeWidget()->createStandardContextMenu(&contextMnu);

	contextMnu.exec(QCursor::pos());
}

void GxsGroupFrameDialog::setStorePostsDelay()
{
    QAction *action = dynamic_cast<QAction*>(sender()) ;

    if(!action || mGroupId.isNull())
    {
        std::cerr << "(EE) Cannot find action/group that called me! Group is " << mGroupId << ", action is " << (void*)action << "  " << __PRETTY_FUNCTION__ << std::endl;
        return;
    }

    uint32_t duration = action->data().toUInt() ;

    std::cerr << "Data is " << duration << std::endl;

 	mInterface->setStoragePeriod(mGroupId,duration * 86400) ;

    // If the sync is larger, we reduce it. No need to sync more than we store. The machinery below also takes care of this.
    //
    uint32_t sync_period = mInterface->getSyncPeriod(mGroupId);

    if(duration > 0)      // the >0 test is to discard the indefinitly test. Basically, if we store for less than indefinitly, the sync is reduced accordingly.
    {
        if(sync_period == 0 || sync_period > duration*86400)
        {
			mInterface->setSyncPeriod(mGroupId,duration * 86400) ;

            std::cerr << "(II) auto adjusting sync period to " << duration<< " days as well." << std::endl;
        }
    }
}


void GxsGroupFrameDialog::setSyncPostsDelay()
{
    QAction *action = dynamic_cast<QAction*>(sender()) ;

    if(!action || mGroupId.isNull())
    {
        std::cerr << "(EE) Cannot find action/group that called me! Group is " << mGroupId << ", action is " << (void*)action << "  " << __PRETTY_FUNCTION__ << std::endl;
        return;
    }

    uint32_t duration = action->data().toUInt() ;

    std::cerr << "Data is " << duration << std::endl;

 	mInterface->setSyncPeriod(mGroupId,duration * 86400) ;

    // If the store is smaller, we increase it accordingly. No need to sync more than we store. The machinery below also takes care of this.
    //
    uint32_t store_period = mInterface->getStoragePeriod(mGroupId);

    if(duration == 0)
 		mInterface->setStoragePeriod(mGroupId,duration * 86400) ;	// indefinite sync => indefinite storage
    else
    {
        if(store_period != 0 && store_period < duration*86400)
        {
 			mInterface->setStoragePeriod(mGroupId,duration * 86400) ;	// indefinite sync => indefinite storage
            std::cerr << "(II) auto adjusting storage period to " << duration<< " days as well." << std::endl;
        }
    }
}

void GxsGroupFrameDialog::restoreGroupKeys(void)
{
	QMessageBox::warning(this, "RetroShare", "ToDo");

#ifdef TOGXS
	mInterface->groupRestoreKeys(mGroupId);
#endif
}

void GxsGroupFrameDialog::newGroup()
{
	GxsGroupDialog *dialog = createNewGroupDialog();

	if (!dialog) {
		return;
	}

	dialog->exec();
	delete(dialog);
}

void GxsGroupFrameDialog::subscribeGroup()
{
	groupSubscribe(true);
}

void GxsGroupFrameDialog::unsubscribeGroup()
{
	groupSubscribe(false);
}

void GxsGroupFrameDialog::groupSubscribe(bool subscribe)
{
	if (mGroupId.isNull()) {
		return;
	}

	uint32_t token;
	mInterface->subscribeToGroup(token, mGroupId, subscribe);
}

void GxsGroupFrameDialog::showGroupDetails()
{
	if (mGroupId.isNull()) {
		return;
	}

	GxsGroupDialog *dialog = createGroupDialog(GxsGroupDialog::MODE_SHOW, mGroupId);
	if (!dialog) {
		return;
	}

	dialog->exec();
	delete(dialog);
}

void GxsGroupFrameDialog::editGroupDetails()
{
	if (mGroupId.isNull()) {
		return;
	}

	GxsGroupDialog *dialog = createGroupDialog(GxsGroupDialog::MODE_EDIT, mGroupId);
	if (!dialog) {
		return;
	}

	dialog->exec();
	delete(dialog);
}

void GxsGroupFrameDialog::copyGroupLink()
{
	if (mGroupId.isNull()) {
		return;
	}

	QString name;
	if(!getCurrentGroupName(name)) return;

	RetroShareLink link = RetroShareLink::createGxsGroupLink(getLinkType(), mGroupId, name);

	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

bool GxsGroupFrameDialog::getCurrentGroupName(QString& name)
{
	return ui->groupTreeWidget->getGroupName(QString::fromStdString(mGroupId.toStdString()), name);
}

void GxsGroupFrameDialog::markMsgAsRead()
{
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId);
	if (msgWidget) {
		msgWidget->setAllMessagesRead(true);
	}
}

void GxsGroupFrameDialog::markMsgAsUnread()
{
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId);
	if (msgWidget) {
		msgWidget->setAllMessagesRead(false);
	}
}

void GxsGroupFrameDialog::sharePublishKey()
{
	if (mGroupId.isNull()) {
		return;
	}

//	QMessageBox::warning(this, "", "ToDo");

    GroupShareKey shareUi(this, mGroupId, shareKeyType());
    shareUi.exec();
}

void GxsGroupFrameDialog::loadComment(const RsGxsGroupId &grpId, const QVector<RsGxsMessageId>& msg_versions, const RsGxsMessageId &most_recent_msgId, const QString &title)
{
	RsGxsCommentService *commentService = getCommentService();
	if (!commentService) {
		/* No comment service available */
		return;
	}

	GxsCommentDialog *commentDialog = commentWidget(most_recent_msgId);

	if (!commentDialog) {
		QString comments = title;
		if (title.length() > MAX_COMMENT_TITLE)
		{
			comments.truncate(MAX_COMMENT_TITLE - 3);
			comments += "...";
		}

        commentDialog = new GxsCommentDialog(this,RsGxsId(), mInterface->getTokenService(), commentService);

		QWidget *commentHeader = createCommentHeaderWidget(grpId, most_recent_msgId);
		if (commentHeader) {
			commentDialog->setCommentHeader(commentHeader);
		}

        std::set<RsGxsMessageId> msgv;
        for(int i=0;i<msg_versions.size();++i)
            msgv.insert(msg_versions[i]);

		commentDialog->commentLoad(grpId, msgv,most_recent_msgId);

		int index = ui->messageTabWidget->addTab(commentDialog, comments);
        ui->messageTabWidget->setTabIcon(index, FilesDefs::getIconFromQtResourcePath(IMAGE_COMMENT));
	}

	ui->messageTabWidget->setCurrentWidget(commentDialog);
}

bool GxsGroupFrameDialog::navigate(const RsGxsGroupId &groupId, const RsGxsMessageId& msgId)
{
	if (groupId.isNull()) {
		return false;
	}

	if (mStateHelper->isLoading(TOKEN_TYPE_GROUP_SUMMARY)) {
		mNavigatePendingGroupId = groupId;
		mNavigatePendingMsgId = msgId;

		/* No information if group is available */
		return true;
	}

	QString groupIdString = QString::fromStdString(groupId.toStdString());
	if (ui->groupTreeWidget->activateId(groupIdString, msgId.isNull()) == NULL) {
		return false;
	}

	changedCurrentGroup(groupIdString);

	/* search exisiting tab */
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId);
	if (!msgWidget) {
		return false;
	}

	if (msgId.isNull()) {
		return true;
	}

	return msgWidget->navigate(msgId);
}

GxsMessageFrameWidget *GxsGroupFrameDialog::messageWidget(const RsGxsGroupId &groupId)
{
	int tabCount = ui->messageTabWidget->count();

	for (int index = 0; index < tabCount; ++index)
    {
		GxsMessageFrameWidget *childWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));

		if (childWidget && childWidget->groupId() == groupId)
			return childWidget;
	}

	return NULL;
}

GxsMessageFrameWidget *GxsGroupFrameDialog::createMessageWidget(const RsGxsGroupId &groupId)
{
	GxsMessageFrameWidget *msgWidget = createMessageFrameWidget(groupId);

	if (!msgWidget)
		return NULL;

	int index = ui->messageTabWidget->addTab(msgWidget, msgWidget->groupName(true));
	ui->messageTabWidget->setTabIcon(index, msgWidget->groupIcon());

	connect(msgWidget, SIGNAL(groupChanged(QWidget*)), this, SLOT(messageTabInfoChanged(QWidget*)));
	connect(msgWidget, SIGNAL(waitingChanged(QWidget*)), this, SLOT(messageTabWaitingChanged(QWidget*)));
	connect(msgWidget, SIGNAL(loadComment(RsGxsGroupId,QVector<RsGxsMessageId>,RsGxsMessageId,QString)), this, SLOT(loadComment(RsGxsGroupId,QVector<RsGxsMessageId>,RsGxsMessageId,QString)));

	return msgWidget;
}

GxsCommentDialog *GxsGroupFrameDialog::commentWidget(const RsGxsMessageId& msgId)
{
	int tabCount = ui->messageTabWidget->count();
	for (int index = 0; index < tabCount; ++index) {
		GxsCommentDialog *childWidget = dynamic_cast<GxsCommentDialog*>(ui->messageTabWidget->widget(index));
		if (childWidget && childWidget->messageId() == msgId) {
			return childWidget;
		}
	}

	return NULL;
}

void GxsGroupFrameDialog::changedCurrentGroup(const QString& groupId)
{
	if (mInFill) {
		return;
	}

	if (groupId.isEmpty())
    {
        auto w = currentWidget();

        if(w)
			w->setGroupId(RsGxsGroupId());

		return;
	}

	mGroupId = RsGxsGroupId(groupId.toStdString());

	if (mGroupId.isNull())
		return;

	/* search exisiting tab */
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId);

    // check that we have at least one tab

	if(msgWidget)
		ui->messageTabWidget->setCurrentWidget(msgWidget);
    else
    {
        if(useTabs() || ui->messageTabWidget->count()==0)
        {
			msgWidget = createMessageWidget(RsGxsGroupId(groupId.toStdString()));
			ui->messageTabWidget->setCurrentWidget(msgWidget);
		}
        else
			currentWidget()->setGroupId(mGroupId);
	}
}

void GxsGroupFrameDialog::groupTreeMiddleButtonClicked(QTreeWidgetItem *item)
{
	openGroupInNewTab(RsGxsGroupId(ui->groupTreeWidget->itemId(item).toStdString()));
}

void GxsGroupFrameDialog::openInNewTab()
{
	openGroupInNewTab(mGroupId);
}

void GxsGroupFrameDialog::openGroupInNewTab(const RsGxsGroupId &groupId)
{
	if (groupId.isNull()) {
		return;
	}

	/* search exisiting tab */
	GxsMessageFrameWidget *msgWidget = createMessageWidget(groupId);

	ui->messageTabWidget->setCurrentWidget(msgWidget);
}

void GxsGroupFrameDialog::messageTabCloseRequested(int index)
{
	if(ui->messageTabWidget->count() == 1) /* Don't close single tab */
		return;

	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
	delete msgWidget ;
}

GxsMessageFrameWidget *GxsGroupFrameDialog::currentWidget() const
{
    return dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(ui->messageTabWidget->currentIndex()));
}

void GxsGroupFrameDialog::messageTabChanged(int index)
{
	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));

	if (!msgWidget)
		return;

	ui->groupTreeWidget->activateId(QString::fromStdString(msgWidget->groupId().toStdString()), false);
}

void GxsGroupFrameDialog::messageTabInfoChanged(QWidget *widget)
{
	int index = ui->messageTabWidget->indexOf(widget);
	if (index < 0) {
		return;
	}

	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
	if (!msgWidget) {
		return;
	}

	ui->messageTabWidget->setTabText(index, msgWidget->groupName(true));
	ui->messageTabWidget->setTabIcon(index, msgWidget->groupIcon());
}

void GxsGroupFrameDialog::messageTabWaitingChanged(QWidget *widget)
{
	int index = ui->messageTabWidget->indexOf(widget);
	if (index < 0) {
		return;
	}

	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
	if (!msgWidget) {
		return;
	}

	ui->groupTreeWidget->setWaiting(QString::fromStdString(msgWidget->groupId().toStdString()), msgWidget->isWaiting());
}

///***** INSERT GROUP LISTS *****/
void GxsGroupFrameDialog::groupInfoToGroupItemInfo(const RsGxsGenericGroupData *groupInfo, GroupItemInfo &groupItemInfo)
{
	groupItemInfo.id = QString::fromStdString(groupInfo->mMeta.mGroupId.toStdString());
	groupItemInfo.name = QString::fromUtf8(groupInfo->mMeta.mGroupName.c_str());
	groupItemInfo.popularity = groupInfo->mMeta.mPop;
	groupItemInfo.lastpost = QDateTime::fromTime_t(groupInfo->mMeta.mLastPost);
	groupItemInfo.subscribeFlags = groupInfo->mMeta.mSubscribeFlags;
	groupItemInfo.publishKey = IS_GROUP_PUBLISHER(groupInfo->mMeta.mSubscribeFlags) ;
	groupItemInfo.adminKey = IS_GROUP_ADMIN(groupInfo->mMeta.mSubscribeFlags) ;
	groupItemInfo.max_visible_posts = groupInfo->mMeta.mVisibleMsgCount ;

#if TOGXS
	if (groupInfo.mGroupFlags & RS_DISTRIB_AUTHEN_REQ) {
		groupItemInfo.name += " (" + tr("AUTHD") + ")";
        groupItemInfo.icon = FilesDefs::getIconFromQtResourcePath(IMAGE_GROUPAUTHD);
	}
	else
#endif
	{
        groupItemInfo.icon = FilesDefs::getIconFromQtResourcePath(icon(ICON_DEFAULT));
	}
}

void GxsGroupFrameDialog::insertGroupsData(const std::list<RsGxsGenericGroupData*>& groupList)
{
	if (!mInitialized) {
		return;
	}

	mInFill = true;

	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for (auto& g:groupList)
    {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = g->mMeta.mSubscribeFlags;

		GroupItemInfo groupItemInfo;
		groupInfoToGroupItemInfo(g, groupItemInfo);

		if (IS_GROUP_SUBSCRIBED(flags))
		{
			if (IS_GROUP_ADMIN(flags))
				adminList.push_back(groupItemInfo);
			else
				subList.push_back(groupItemInfo); /* subscribed group */
		}
		else
        {
			popMap.insert(std::make_pair(g->mMeta.mLastPost, groupItemInfo)); /* rate the others by time of last post */
        }
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10)
		popCount = popMap.size() / 10;

	uint32_t i = 0;
	std::multimap<uint32_t, GroupItemInfo>::reverse_iterator rit;

	for (rit = popMap.rbegin(); rit != popMap.rend(); ++rit,++i)
		if(i < popCount)
			popList.append(rit->second);
		else
			otherList.append(rit->second);

	/* now we can add them in as a tree! */
	ui->groupTreeWidget->fillGroupItems(mYourGroups, adminList);
	mYourGroups->setText(2, QString::number(mYourGroups->childCount())); 
	ui->groupTreeWidget->fillGroupItems(mSubscribedGroups, subList);
	mSubscribedGroups->setText(2, QString::number(mSubscribedGroups->childCount())); // 1 COLUMN_UNREAD 2 COLUMN_POPULARITY
	ui->groupTreeWidget->fillGroupItems(mPopularGroups, popList);
	mPopularGroups->setText(2, QString::number(mPopularGroups->childCount()));
	ui->groupTreeWidget->fillGroupItems(mOtherGroups, otherList);
	mOtherGroups->setText(2, QString::number(mOtherGroups->childCount()));

	mInFill = false;

	/* Re-fill group */
	if (!ui->groupTreeWidget->activateId(QString::fromStdString(mGroupId.toStdString()), true))
		mGroupId.clear();

	updateMessageSummaryList(RsGxsGroupId());
}

void GxsGroupFrameDialog::updateMessageSummaryList(RsGxsGroupId groupId)
{
    // groupId.isNull() means that we need to update all groups so we clear up the list of groups to update.

    if(!groupId.isNull())
        mGroupIdsSummaryToUpdate.insert(groupId);
    else
        mGroupIdsSummaryToUpdate.clear();

    mShouldUpdateMessageSummaryList = true;
}

void GxsGroupFrameDialog::updateMessageSummaryListReal(RsGxsGroupId groupId)
{
	if (!mInitialized) {
		return;
	}

	if (groupId.isNull())
    {
		QTreeWidgetItem *items[2] = { mYourGroups, mSubscribedGroups };
		for (int item = 0; item < 2; ++item) {
			int child;
			int childCount = items[item]->childCount();
			for (child = 0; child < childCount; ++child) {
				QTreeWidgetItem *childItem = items[item]->child(child);
				QString childId = ui->groupTreeWidget->itemId(childItem);
				if (childId.isEmpty())
					continue;

				updateGroupStatistics(RsGxsGroupId(childId.toLatin1().constData()));
			}
		}
	}
    else
		updateGroupStatistics(groupId);
}

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

void GxsGroupFrameDialog::updateGroupSummary()
{
	RsThread::async([this]()
	{
		auto groupInfo = new std::list<RsGxsGenericGroupData*>() ;

		if(!getGroupData(*groupInfo))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to collect group info." << std::endl;
            delete groupInfo;
			return;
		}
        if(groupInfo->empty())
        {
			std::cerr << __PRETTY_FUNCTION__ << " no group info collected." << std::endl;
            delete groupInfo;
            return;
        }

		RsQThreadUtils::postToObject( [this,groupInfo]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

			insertGroupsData(*groupInfo);
			updateSearchResults();

			mStateHelper->setLoading(TOKEN_TYPE_GROUP_SUMMARY, false);

			if (!mNavigatePendingGroupId.isNull()) {
				/* Navigate pending */
				navigate(mNavigatePendingGroupId, mNavigatePendingMsgId);

				mNavigatePendingGroupId.clear();
				mNavigatePendingMsgId.clear();
			}

			// update the local cache in order to avoid re-asking the data when the UI wants it (this happens on ::show() for instance)

			mCachedGroupMetas.clear();

			// now delete the data that is not used anymore

			for(auto& g:*groupInfo)
			{
				mCachedGroupMetas[g->mMeta.mGroupId] = g->mMeta;
				delete g;
			}

            delete groupInfo;

		}, this );
	});
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsGroupFrameDialog::updateGroupStatistics(const RsGxsGroupId &groupId)
{
    mGroupStatisticsToUpdate.insert(groupId);
    mShouldUpdateGroupStatistics = true;
}

void GxsGroupFrameDialog::updateGroupStatisticsReal(const RsGxsGroupId &groupId)
{
    RsThread::async([this,groupId]()
	{
		GxsGroupStatistic stats;

        if(! getGroupStatistics(groupId, stats))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to collect group statistics for group " << groupId << std::endl;
			return;
		}

		RsQThreadUtils::postToObject( [this,stats, groupId]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

			QTreeWidgetItem *item = ui->groupTreeWidget->getItemFromId(QString::fromStdString(stats.mGrpId.toStdString()));
			if (!item)
				return;

			ui->groupTreeWidget->setUnreadCount(item, mCountChildMsgs ? (stats.mNumThreadMsgsUnread + stats.mNumChildMsgsUnread) : stats.mNumThreadMsgsUnread);
            mCachedGroupStats[groupId] = stats;

			getUserNotify()->updateIcon();

		}, this );
	});
}

void GxsGroupFrameDialog::getServiceStatistics(GxsServiceStatistic& stats) const
{
	stats = GxsServiceStatistic(); // clears everything

   for(auto it:  mCachedGroupStats)
   {
       const GxsGroupStatistic& s(it.second);

	   stats.mNumMsgs             += s.mNumMsgs;
	   stats.mNumGrps             += 1;
	   stats.mSizeOfMsgs          += s.mTotalSizeOfMsgs;
	   stats.mNumThreadMsgsNew    += s.mNumThreadMsgsNew;
	   stats.mNumThreadMsgsUnread += s.mNumThreadMsgsUnread;
	   stats.mNumChildMsgsNew     += s.mNumChildMsgsNew ;
	   stats.mNumChildMsgsUnread  += s.mNumChildMsgsUnread ;
   }
}

TurtleRequestId GxsGroupFrameDialog::distantSearch(const QString& search_string)   // this should be overloaded in the child class
{
    std::cerr << "Searching for \"" << search_string.toStdString() << "\". Function is not overloaded, so nothing will happen." << std::endl;
    return 0;
}

void GxsGroupFrameDialog::searchNetwork(const QString& search_string)
{
    if(search_string.isNull())
        return ;

    uint32_t request_id = distantSearch(search_string);

    if(request_id == 0)
        return ;

    mSearchGroupsItems[request_id] = ui->groupTreeWidget->addSearchItem(tr("Search for")+ " \"" + search_string + "\"",(uint32_t)request_id,FilesDefs::getIconFromQtResourcePath(icon(ICON_SEARCH)));
}

void GxsGroupFrameDialog::distantRequestGroupData()
{
    QAction *action = dynamic_cast<QAction*>(sender()) ;

    if(!action)
        return ;

    RsGxsGroupId group_id(action->data().toString().toStdString());

    if(group_id.isNull())
    {
        std::cerr << "Cannot retrieve group! Group id is null!" << std::endl;
    }

	std::cerr << "Explicit request for group " << group_id << std::endl;
    checkRequestGroup(group_id) ;
}

