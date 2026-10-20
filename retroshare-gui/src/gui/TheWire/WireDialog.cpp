/*******************************************************************************
 * gui/TheWire/WireDialog.cpp                                                  *
 *                                                                             *
 * Copyright (c) 2012-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#include "WireDialog.h"

#include "WireGroupDialog.h"
#include "WireGroupItem.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "gui/gxs/GxsStatisticsProvider.h"
#include "gui/gxs/GxsGroupShareKey.h"

#include "PulseViewGroup.h"
#include "PulseReplySeperator.h"
#include "WireUserNotify.h"

#include "util/qtthreadsutils.h"
#include "util/misc.h"

#include <retroshare/rspeers.h>
#include <retroshare/rswire.h>

#include <algorithm>
#include <iostream>
#include <sstream>

#include <QTimer>
#include <QMessageBox>
#include <QMenu>
#include <QActionGroup>
#include <QTabBar>
#include <QComboBox>
#include <QTextEdit>
#include <QTextDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollBar>
#include <QResizeEvent>
#include <QRegularExpression>
#include <QSet>
#include <QMap>

/****************************************************************
 * TheWire Display Widget.
 *
 */

#define GROUP_SET_ALL           (0)
#define GROUP_SET_OWN           (1)
#define GROUP_SET_SUBSCRIBED    (2)
#define GROUP_SET_OTHERS        (3)
// Future Extensions.
// #define GROUP_SET_AUTO          (4)
// #define GROUP_SET_RECOMMENDED   (5)

// Feed tabs above the pulses, mapped to the group sets of the combo box.
static const int sFeedTabGroupSets[] = { GROUP_SET_SUBSCRIBED, GROUP_SET_ALL, GROUP_SET_OWN, GROUP_SET_OTHERS };
static const int sFeedTabCount = 4;

#define WIRE_FEED_WIDTH          600
#define WIRE_SIDEBAR_MIN_WINDOW  1000  // hide the right sidebar below this window width


static int groupSetToFeedTab(int groupSet)
{
	for (int i = 0; i < sFeedTabCount; ++i) {
		if (sFeedTabGroupSets[i] == groupSet) {
			return i;
		}
	}
	return 0;
}

static void clearWireLayout(QLayout *layout)
{
	if (!layout) {
		return;
	}
	QLayoutItem *item;
	while ((item = layout->takeAt(0)) != nullptr) {
		if (item->widget()) {
			item->widget()->deleteLater();
		}
		delete item;
	}
}

// same font as the "Post" button of PulseAddDialog.ui (12pt bold).
static void setPostButtonFont(QPushButton *button)
{
	QFont f = button->font();
	f.setPointSize(12);
	f.setBold(true);
	button->setFont(f);
}

static QFrame *createSidebarCard(const QString &title, QVBoxLayout **contentLayout)
{
	QFrame *card = new QFrame();
	card->setObjectName("wireSidebarCard");

	QVBoxLayout *layout = new QVBoxLayout(card);
	layout->setContentsMargins(14, 10, 14, 12);
	layout->setSpacing(6);

	QLabel *titleLabel = new QLabel(title);
	QFont f = titleLabel->font();
	f.setBold(true);
	f.setPointSizeF(f.pointSizeF() * 1.35);
	titleLabel->setFont(f);
	titleLabel->setObjectName("wireSidebarTitle");
	layout->addWidget(titleLabel);

	QVBoxLayout *content = new QVBoxLayout();
	content->setContentsMargins(0, 0, 0, 0);
	content->setSpacing(2);
	layout->addLayout(content);

	*contentLayout = content;
	return card;
}


/** Constructor */
WireDialog::WireDialog(QWidget *parent)
    : MainPage(parent), mGroupSet(GROUP_SET_ALL)
    , mAddDialog(nullptr), mGroupSelected(nullptr)
    , mHistoryIndex(-1), mEventHandlerId(0), mSortAscending(false)
    , mAccountSortType(0), mHideInactiveAccounts(false)
{
	ui.setupUi(this);

	mInterface = rsWire;
	mSettingsName = "WireDialog";
	mDistSyncAllowed = true;
	mStateHelper = nullptr;

	mRequestGroupDataInProgress = false;
	mUpdateTimer = new QTimer(this);
	mUpdateTimer->setSingleShot(true);
	connect(mUpdateTimer, SIGNAL(timeout()), this, SLOT(refreshGroups()));

	connect( ui.toolButton_createAccount, SIGNAL(clicked()), this, SLOT(createGroup()));
	connect( ui.toolButton_createPulse, SIGNAL(clicked()), this, SLOT(createPulse()));
	connect( ui.toolButton_home, SIGNAL(clicked()), this, SLOT(showHomeFeed()));
	connect( ui.lineEdit_searchUsers, SIGNAL(textChanged(const QString&)), this, SLOT(filterUsers(const QString&)));

	connect(ui.comboBox_groupSet, SIGNAL(currentIndexChanged(int)), this, SLOT(selectGroupSet(int)));
	connect(ui.comboBox_filterTime, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFilterTime(int)));

	connect( ui.toolButton_back, SIGNAL(clicked()), this, SLOT(back()));
	connect( ui.toolButton_forward, SIGNAL(clicked()), this, SLOT(forward()));
	ui.toolButton_back->setEnabled(false);
	ui.toolButton_forward->setEnabled(false);

	// Setup filter menu
	QMenu *filterMenu = new QMenu(this);
	QAction *sortAscAction = filterMenu->addAction(tr("Sort by Ascending"));
	sortAscAction->setCheckable(true);
	sortAscAction->setChecked(mSortAscending);
	connect(sortAscAction, SIGNAL(triggered()), this, SLOT(toggleSortAscending()));

	ui.toolButton_filter->setMenu(filterMenu);

	// Setup account filter menu
	QMenu *accountMenu = new QMenu(this);
	QActionGroup *sortGroup = new QActionGroup(this);

	sortByName = accountMenu->addAction(tr("Sort by Name"));
	sortByName->setCheckable(true);
	sortByName->setChecked(true);
	sortByName->setData(0);
	sortGroup->addAction(sortByName);

	sortByActivity = accountMenu->addAction(tr("Sort by Activity"));
	sortByActivity->setCheckable(true);
	sortByActivity->setData(1);
	sortGroup->addAction(sortByActivity);

	accountMenu->addSeparator();

	QAction *hideInactiveAction = accountMenu->addAction(tr("Hide inactive (>30d)"));
	hideInactiveAction->setCheckable(true);
	hideInactiveAction->setChecked(false);

	connect(sortGroup, SIGNAL(triggered(QAction*)), this, SLOT(sortAccountsChanged(QAction*)));
	connect(hideInactiveAction, SIGNAL(toggled(bool)), this, SLOT(hideInactiveChanged(bool)));

	ui.toolButton_accountFilter->setMenu(accountMenu);

	// Twitter-like layout (feed column, tabs, compose box, right sidebar).
	// Must happen before processSettings() which restores the splitter.
	setupFeedLayout();
	setupComposeBox();
	setupSidebar();
	setupLeftNavigation();
	setupReplyBox();

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	mCountChildMsgs = false;
	mShouldUpdateGroupStatistics = false;
	mDistSyncAllowed = true;

	requestGroupData();

	// just for testing (disabled: it showed empty pulse cards at startup)
	// postTestTwitterView();

	// load settings
	processSettings(true);

    // Needs to be asynced because this function is called by another thread!
    rsEvents->registerEventsHandler(
                [this](std::shared_ptr<const RsEvent> event)
    { RsQThreadUtils::postToObject([=]() { handleEvent_main_thread(event); }, this ); },
                mEventHandlerId, RsEventType::WIRE );
}

void WireDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    const RsWireEvent *e = dynamic_cast<const RsWireEvent*>(event.get());

    if(e)
    {

#ifdef GXSWIRE_DEBUG
                RsDbg() << " Refreshing the feed if there is a matching event. "<< std::endl;
#endif

        // The following switch statements refresh the wire feed whenever there is a new event

        switch(e->mWireEventCode)
        {

        case RsWireEventCode::READ_STATUS_CHANGED:
            updateGroupStatisticsReal(e->mWireGroupId); // update the list immediately
            return;

        case RsWireEventCode::STATISTICS_CHANGED:
        case RsWireEventCode::NEW_POST:
        case RsWireEventCode::NEW_REPLY:
        case RsWireEventCode::NEW_LIKE:
        case RsWireEventCode::NEW_REPUBLISH:
        case RsWireEventCode::POST_UPDATED:
        case RsWireEventCode::FOLLOW_STATUS_CHANGED:
        case RsWireEventCode::NEW_WIRE:
            updateGroupStatisticsReal(e->mWireGroupId);
            break;

        default:
#ifdef GXSWIRE_DEBUG
                RsDbg() << " Unknown event occured: "<< e->mWireEventCode << std::endl;
#endif
            break;

        }
        if(!mUpdateTimer->isActive()) mUpdateTimer->start(1000);
    }
}

WireDialog::~WireDialog()
{
	// save settings
	processSettings(false);
	
	clearTwitterView();

    rsEvents->unregisterEventsHandler(mEventHandlerId);
}

UserNotify *WireDialog::createUserNotify(QObject *parent)
{
    return new WireUserNotify(rsWire, this, parent);
}

//QString WireDialog::getHelpString() const
//{
//    int H = misc::getFontSizeFactor("HelpButton").height();

//    QString hlp_str = tr(
//        "<h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Boards</h1>"
//        "<p>The wire service is a day to day social network service.</p>"
//        "<p>The Wire service allows you to share your pulses, republish someone else's pulses, like "
//        "a pulse.</p>"
//        "<p>A pulse is like a channel or board post. It can be a image or a text or both. You can "
//        "only see the pulses of the people you follow.</p>"
//        "<p>Wire posts are kept for %2 days, and sync-ed over the last %3 days, unless you change this.</p>"
//                        ).arg(  QString::number(2*H)
//                              , QString::number(rsWire->getDefaultStoragePeriod()/86400)
//                              , QString::number(rsWire->getDefaultSyncPeriod()/86400));

//    return hlp_str ;
//}

//QString WireDialog::text(TextType type)
//{
//    switch (type) {
//    case TEXT_NAME:
//        return tr("Wire");
//    case TEXT_NEW:
//        return tr("Create Wire");

//// Dont know what this does
////	case TEXT_TODO:
////		return "<b>Open points:</b><ul>"
////		       "<li>Subreddits/tag to posts support"
////		       "<li>Picture Support"
////		       "<li>Navigate channel link"
////		       "</ul>";

//    case TEXT_YOUR_GROUP:
//        return tr("My Wire");
//    case TEXT_SUBSCRIBED_GROUP:
//        return tr("Followed Wires");
//    case TEXT_POPULAR_GROUP:
//        return tr("Popular Wires");
//    case TEXT_OTHER_GROUP:
//        return tr("Other Wires");
//    }

//    return "";
//}

//QString WireDialog::icon(IconType type)
//{
//    switch (type) {
//    case ICON_NAME:
//        return ":/icons/png/postedlinks.png";
//    case ICON_NEW:
//        return ":/icons/png/add.png";
//    case ICON_YOUR_GROUP:
//        return "";
//    case ICON_SUBSCRIBED_GROUP:
//        return "";
//    case ICON_POPULAR_GROUP:
//        return "";
//    case ICON_OTHER_GROUP:
//        return "";
//    case ICON_SEARCH:
//        return ":/images/find.png";
//    case ICON_DEFAULT:
//        return ":/icons/png/posted.png";
//    }

//    return "";
//}

//bool WireDialog::getGroupData(std::list<RsGxsGenericGroupData*>& groupInfo)
//{
//    std::vector<RsWireGroup> groups;

//    // request all group infos at once

//    if(! rsWire->getGroups(std::list<RsGxsGroupId>(),groups))
//        return false;

//    /* Save groups to fill icons and description */

//    for (auto& group: groups)
//       groupInfo.push_back(new RsWireGroup(group));

//    return true;
//}

bool WireDialog::getGroupStatistics(const RsGxsGroupId& groupId,GxsGroupStatistic& stat)
{
    // What follows is a hack to replace the GXS group statistics by the actual count of unread messages in wire,
    // which should take into account old post versions, discard replies and likes, etc.

    RsWireStatistics s;
    bool res = rsWire->getWireStatistics(groupId,s);

    if(!res)
        return false;

    stat.mGrpId = groupId;
    stat.mNumMsgs = s.mNumberOfPulses;

    stat.mTotalSizeOfMsgs = 0;	// hopefuly unused. Required the loading of the full channel data, so not very convenient.
    stat.mNumThreadMsgsNew = s.mNumberOfNewPulses;
    stat.mNumThreadMsgsUnread = s.mNumberOfUnreadPulses;
    stat.mNumChildMsgsNew = 0;
    stat.mNumChildMsgsUnread = 0;

    return true;
}

//GxsGroupDialog *WireDialog::createNewGroupDialog()
//{
//    return new WireGroupDialog(this);
//}

//GxsGroupDialog *WireDialog::createGroupDialog(GxsGroupDialog::Mode mode, RsGxsGroupId groupId)
//{
//    return new WireGroupDialog(mode, groupId, this);
//}

//int WireDialog::shareKeyType()
//{
//    return GroupShareKey::NO_KEY_SHARE;
//}

//GxsMessageFrameWidget *WireDialog::createMessageFrameWidget(const RsGxsGroupId &groupId)
//{
//    // to do
////	return new WireListWidgetWithModel(groupId);
//}

void WireDialog::processSettings(bool load)
{
	Settings->beginGroup("WireDialog");

	if (load) {
		// load settings

		// state of splitter
		// new key: the splitter has 3 widgets now (groups | feed | sidebar)
		QByteArray splitterState = Settings->value("SplitterWire3").toByteArray();
		if (!splitterState.isEmpty()) {
			ui.splitter->restoreState(splitterState);
		}

		// state of filter combobox
		int index = Settings->value("ShowGroup", 0).toInt();
		ui.comboBox_groupSet->setCurrentIndex(index);

		// state of sort filter
		sortByActivity->setChecked(Settings->value("sortByActivity", true).toBool());
		sortByName->setChecked(Settings->value("SortByName", true).toBool());
	} else {
		// save settings

		// state of filter combobox
		Settings->setValue("ShowGroup", ui.comboBox_groupSet->currentIndex());

		// state of splitter
		Settings->setValue("SplitterWire3", ui.splitter->saveState());

		// state of sort filter
		Settings->setValue("SortByActivity", sortByActivity->isChecked());
		Settings->setValue("SortByName", sortByName->isChecked());
	}

	Settings->endGroup();
}

void WireDialog::refreshGroups()
{
	requestGroupData();
}

void WireDialog::showHomeFeed()
{
	mGroupSet = GROUP_SET_SUBSCRIBED;
	ui.comboBox_groupSet->setCurrentIndex(GROUP_SET_SUBSCRIBED);
	mGroupSelected = nullptr;
	showGroups();
}

void WireDialog::filterUsers(const QString &text)
{
	QLayout *alayout = ui.groupsWidget->layout();
	QString filterText = text.toLower();

	for (int i = 0; i < alayout->count(); ++i)
	{
		QLayoutItem *item = alayout->itemAt(i);
		QWidget *widget = item->widget();
		WireGroupItem *groupItem = dynamic_cast<WireGroupItem *>(widget);

		if (groupItem)
		{
			bool visible = filterText.isEmpty() || groupItem->matchesFilter(filterText);
			groupItem->setVisible(visible);
		}
	}
}

void WireDialog::addGroup(QWidget *item)
{
	// insert before the trailing spacer so the list starts at the top.
	QBoxLayout *boxlayout = dynamic_cast<QBoxLayout *>(ui.groupsWidget->layout());
	if (!boxlayout) {
		ui.groupsWidget->layout()->addWidget(item);
		return;
	}
	int pos = boxlayout->count();
	if (pos > 0 && boxlayout->itemAt(pos - 1)->spacerItem()) {
		--pos;
	}
	boxlayout->insertWidget(pos, item);
}

bool WireDialog::setupPulseAddDialog()
{
	std::cerr << "WireDialog::setupPulseAddDialog()";
	std::cerr << std::endl;

	if (!mAddDialog)
	{
		mAddDialog = new PulseAddDialog(NULL);
		mAddDialog->hide();
	}

	mAddDialog->cleanup();

	int idx = ui.groupChooser->currentIndex();
	if (idx < 0 || idx >= (int)mOwnGroups.size()) {
		std::cerr << "WireDialog::setupPulseAddDialog() ERROR GETTING AuthorId!";
		std::cerr << std::endl;

		QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose Wire Groupd first"), QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}

	RsWireGroup group = mOwnGroups[idx];
	mAddDialog->setGroup(group);

	return true;
}


void WireDialog::subscribe(RsGxsGroupId &groupId)
{
	RsThread::async([groupId]()
	{
		rsWire->subscribe(groupId, true);
	});
}

void WireDialog::unsubscribe(RsGxsGroupId &groupId)
{
	RsThread::async([groupId]()
	{
		rsWire->subscribe(groupId, false);
	});
}

void WireDialog::notifyGroupSelection(WireGroupItem *item)
{
	std::cerr << "WireDialog::notifyGroupSelection() from : " << item;
	std::cerr << std::endl;

	bool doSelection = true;
	if (mGroupSelected)
	{
		std::cerr << "WireDialog::notifyGroupSelection() unselecting old one : " << mGroupSelected;
		std::cerr << std::endl;

		mGroupSelected->setSelected(false);
		if (mGroupSelected == item)
		{
			std::cerr << "WireDialog::notifyGroupSelection() current -> unselect";
			std::cerr << std::endl;
			/* de-selection of current item */
			mGroupSelected = NULL;
			doSelection = false;
		}
	}

	if (doSelection)
	{
		item->setSelected(true);
		mGroupSelected = item;
	}

	/* update display */
	showSelectedGroups();
}


void WireDialog::checkUpdate()
{
#if 0
	/* update */
	if (!rsWire)
		return;

	if (rsWire->updated())
	{
		insertAlbums();
	}
#endif
	return;
}

void WireDialog::createGroup()
{
	WireGroupDialog wireCreate(this);
	wireCreate.exec();
}

void WireDialog::createPulse()
{
	if (!mAddDialog)
	{
		mAddDialog = new PulseAddDialog(NULL);
		mAddDialog->hide();
	}

	int idx = ui.groupChooser->currentIndex();
	if (idx < 0) {
		std::cerr << "WireDialog::createPulse() ERROR GETTING AuthorId!";
		std::cerr << std::endl;

		QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose Wire Groupd first"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	RsWireGroup group = mOwnGroups[idx];

	mAddDialog->cleanup();
	mAddDialog->setGroup(group.mMeta.mGroupId);
	mAddDialog->show();
}

void WireDialog::addGroup(const RsWireGroup &group)
{
	std::cerr << "WireDialog::addGroup() GroupId : " << group.mMeta.mGroupId;
	std::cerr << std::endl;

	WireGroupItem *item = new WireGroupItem(this, group);

	// Apply cached unread count so the badge is correct from the moment the item appears
	auto sit = mCachedGroupStats.find(group.mMeta.mGroupId);
	if (sit != mCachedGroupStats.end())
		item->setUnreadCount(sit->second.mNumThreadMsgsUnread);

	addGroup(item);
}

void WireDialog::deleteGroups()
{
	std::cerr << "WireDialog::deleteGroups()";
	std::cerr << std::endl;

	mGroupSelected = NULL;

	QLayout *alayout = ui.groupsWidget->layout();
	QLayoutItem *item;
	int i = 0;
	while (i < alayout->count())
	{
		item = alayout->itemAt(i);
		QWidget *widget = item->widget();
		if (NULL != dynamic_cast<WireGroupItem *>(widget))
		{
			std::cerr << "WireDialog::deleteGroups() Removing Item at: " << i;
			std::cerr << std::endl;

			item = alayout->takeAt(i);
			item->widget()->deleteLater();
			delete item;
		}
		else
		{
			std::cerr << "WireDialog::deleteGroups() Leaving Item at: " << i;
			std::cerr << std::endl;

			i++;
		}
	}
}

void WireDialog::updateGroups(const std::vector<RsWireGroup>& groups)
{
	// remember which account was selected in the "Post as" chooser.
	RsGxsGroupId selectedOwnGroup;
	int selectedIdx = ui.groupChooser->currentIndex();
	if (selectedIdx >= 0 && selectedIdx < (int)mOwnGroups.size()) {
		selectedOwnGroup = mOwnGroups[selectedIdx].mMeta.mGroupId;
	}

	mAllGroups.clear();
	mOwnGroups.clear();
	ui.groupChooser->blockSignals(true);
	ui.groupChooser->clear();

	for(auto &it : groups) {
		// save list of all groups.
		mAllGroups[it.mMeta.mGroupId] = it;

		// NB: mOwnGroups must only hold ADMIN groups, its index has to match
		// ui.groupChooser (previously followed groups were added here too,
		// which made "Post Pulse" use the wrong account).

        addGroup(it);

		if (it.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
		{
			// grab own groups.
			// setup Chooser too.
			mOwnGroups.push_back(it); 
			// same picture as the avatar of the compose box: headshot, else the default group icon.
			QPixmap pixmap;
			if (it.mHeadshot.mData) {
				GxsIdDetails::loadPixmapFromData(it.mHeadshot.mData, it.mHeadshot.mSize, pixmap, GxsIdDetails::ORIGINAL);
			}
			if (pixmap.isNull()) {
				pixmap = GxsIdDetails::makeDefaultGroupIcon(it.mMeta.mGroupId, IMAGE_WIRE, GxsIdDetails::ORIGINAL);
			}

			ui.groupChooser->addItem(QIcon(WireRoundPixmap(pixmap, 32)), QString::fromStdString(it.mMeta.mGroupName));
		}
	}

	for (int i = 0; i < (int)mOwnGroups.size(); ++i) {
		if (mOwnGroups[i].mMeta.mGroupId == selectedOwnGroup) {
			ui.groupChooser->setCurrentIndex(i);
			break;
		}
	}
	ui.groupChooser->blockSignals(false);
	composeAccountChanged(ui.groupChooser->currentIndex());

    updateGroupCountLabels();
    updateGroupsToFollow();
    updateNetworkCard();
}


void WireDialog::selectGroupSet(int index)
{
	std::cerr << "WireDialog::selectGroupSet(" << index << ")";
	std::cerr << std::endl;

	mGroupSet = index;
	if (mGroupSet < 0) {
		mGroupSet = GROUP_SET_ALL;
	}

	// keep the feed tabs in sync with the combo box.
	if (mFeedTabs) {
		mFeedTabs->blockSignals(true);
		mFeedTabs->setCurrentIndex(groupSetToFeedTab(mGroupSet));
		mFeedTabs->blockSignals(false);
	}

	// show follow label and account filter only when All is selected
	bool showFollow = (mGroupSet == GROUP_SET_ALL);
	ui.followLabel->setVisible(showFollow);
	ui.toolButton_accountFilter->setVisible(showFollow);

	showGroups();
}

void WireDialog::selectFilterTime(int index)
{
	std::cerr << "WireDialog::selectFilterTime(" << index << ")";
	std::cerr << std::endl;

	showSelectedGroups();
}

void WireDialog::toggleSortAscending()
{
	mSortAscending = !mSortAscending;
	showSelectedGroups();
}

void WireDialog::sortAccountsChanged(QAction *action)
{
	mAccountSortType = action->data().toInt();
	showGroups();
}

void WireDialog::hideInactiveChanged(bool checked)
{
	mHideInactiveAccounts = checked;
	showGroups();
}

void WireDialog::showSelectedGroups()
{
	if (mGroupSelected)
	{
		// request data.
		std::list<RsGxsGroupId> grpIds;
		grpIds.push_back(mGroupSelected->groupId());

		// show GroupFocus.
		requestGroupFocus(mGroupSelected->groupId());
	}
	else
	{
		showGroups();
	}
}

void WireDialog::showGroups(bool addToHistory, bool loadPulses)
{
	deleteGroups();

	std::list<RsGxsGroupId> allGroupIds;
	std::vector<RsWireGroup> groupsToShow;

	// Collect groups that match the current filter
	rstime_t now = time(nullptr);
	rstime_t thirtyDaysAgo = now - (30 * 24 * 60 * 60);

	for (auto &it : mAllGroups)
	{
		bool add = (mGroupSet == GROUP_SET_ALL);
		if (it.second.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
			if (mGroupSet == GROUP_SET_OWN) {
				add = true;
			}
		}
		else if (it.second.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) {
			if (mGroupSet == GROUP_SET_SUBSCRIBED) {
				add = true;
			}
		}
		else {
			if (mGroupSet == GROUP_SET_OTHERS) {
				add = true;
			}
		}

		// Filter inactive accounts if checkbox is checked
		if (add && mHideInactiveAccounts) {
			rstime_t lastPost = it.second.mMeta.mLastPost;
			if (lastPost == 0 || lastPost < thirtyDaysAgo) {
				add = false;
			}
		}

		if (add) {
			groupsToShow.push_back(it.second);
		}
	}

	// Sort groups based on selection
	if (mAccountSortType == 0) {
		// Sort by name (alphabetical)
		std::sort(groupsToShow.begin(), groupsToShow.end(),
			[](const RsWireGroup &a, const RsWireGroup &b) {
				return QString::fromStdString(a.mMeta.mGroupName).toLower() <
				       QString::fromStdString(b.mMeta.mGroupName).toLower();
			});
	} else {
		// Sort by activity (most recent first)
		std::sort(groupsToShow.begin(), groupsToShow.end(),
			[](const RsWireGroup &a, const RsWireGroup &b) {
				return a.mMeta.mLastPost > b.mMeta.mLastPost;
			});
	}

	// Add sorted groups to UI
	for (auto &group : groupsToShow) {
		addGroup(group);
		allGroupIds.push_back(group.mMeta.mGroupId);
	}

	if (loadPulses) {
		requestGroupsPulses(allGroupIds, addToHistory);
	}
}


// LOAD DATA...............................................

void WireDialog::requestGroupData()
{
	std::cerr << "WireDialog::requestGroupData()";
	std::cerr << std::endl;

	if (mRequestGroupDataInProgress) {
		return;
	}

	mRequestGroupDataInProgress = true;

	RsThread::async([this]()
	{
		std::vector<RsWireGroup> groups;
		rsWire->getGroups({}, groups);

		RsQThreadUtils::postToObject([this, groups]()
		{
			mRequestGroupDataInProgress = false;
			updateGroups(groups);

			// Background update (new pulse, reply, like...): refresh the view the user
			// is looking at instead of always jumping back to the feed, and keep the scroll position.
			mRestoreScroll = ui.scrollArea->verticalScrollBar()->value();

			bool haveView = (mHistoryIndex >= 0 && mHistoryIndex < (int)mHistory.size());
			WireViewHistory current;
			if (haveView) {
				current = mHistory[mHistoryIndex];
			}

			if (haveView && current.viewType == WireViewType::PULSE_FOCUS)
			{
				showGroups(false, false); // account list only
				showPulseFocus(current.groupId, current.msgId);
			}
			else if (haveView && current.viewType == WireViewType::GROUP_FOCUS)
			{
				showGroups(false, false); // account list only
				WireGroupItem *item = findGroupItemWidget(current.groupId);
				if (item) {
					item->setSelected(true);
					mGroupSelected = item;
				}
				showGroupFocus(current.groupId);
			}
			else
			{
				showGroups(false); // Background update, don't add to history
			}

			if (!mNavigatePendingGroupId.isNull()) {
				if (!mNavigatePendingMsgId.isNull()) {
					requestPulseFocus(mNavigatePendingGroupId, mNavigatePendingMsgId);
				} else {
					requestGroupFocus(mNavigatePendingGroupId);
				}
				mNavigatePendingGroupId.clear();
				mNavigatePendingMsgId.clear();
			}
		}, this);
	});
}

rstime_t WireDialog::getFilterTimestamp()
{
	rstime_t filterTimestamp = time(NULL);
	switch(ui.comboBox_filterTime->currentIndex())
	{
		case 1: // Last 24 Hours.
			filterTimestamp -= (3600 * 24);
			break;
		case 2: // Last 7 Days.
			filterTimestamp -= (3600 * 24 * 7);
			break;
		case 3: // Last 30 Days.
			filterTimestamp -= (3600 * 24 * 30);
			break;
		case 0: // All Time.
		case -1: // no index.
		default:
			filterTimestamp = 0; // back to Epoch! effectively all.
			break;
	}
	return filterTimestamp;
}


/****************************************************************************************/
// TWITTER VIEW.
/****************************************************************************************/

// TODO
// - Handle Groups
//	 - Add GroupPtr to WirePulseSPtrs (DONE)
//   - Add HeadShot to WireGroup
//   - Add GxsIdLabel to Pulse UI elements.
//
// - Create Groups.
//   - Add HeadShot
//
// - Create Pulse.
//   - Add Images.
//
// - Link up Reply / Republish / Like.
//
// - showGroupFocus
//   - TODO
//
// - showPulseFocus
//   - Basics (DONE).
//   - MoreReplies
//   - Show Actual Message.

//
// - showReplyFocus
//   - TODO


// PulseDataItem interface
// Actions.
void WireDialog::PVHreply(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHreply() GroupId: " << groupId;
	std::cerr << " MsgId: " << msgId;
	std::cerr << std::endl;

	if (setupPulseAddDialog())
	{
		mAddDialog->setReplyTo(groupId, msgId, WIRE_PULSE_TYPE_REPLY);
	}
}

void WireDialog::PVHrepublish(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHrepublish() GroupId: " << groupId;
	std::cerr << " MsgId: " << msgId;
	std::cerr << std::endl;

	if (setupPulseAddDialog())
	{
		mAddDialog->setReplyTo(groupId, msgId, WIRE_PULSE_TYPE_REPUBLISH);
	}
}

void WireDialog::PVHlike(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHlike() GroupId: " << groupId;
	std::cerr << " MsgId: " << msgId;
	std::cerr << std::endl;

	if (setupPulseAddDialog())
	{
		mAddDialog->setReplyTo(groupId, msgId, WIRE_PULSE_TYPE_LIKE);
	}
}

void WireDialog::PVHviewGroup(const RsGxsGroupId &groupId)
{
	std::cerr << "WireDialog::PVHviewGroup(";
	std::cerr << groupId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	requestGroupFocus(groupId);
}

void WireDialog::PVHviewPulse(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHviewPulse(";
	std::cerr << groupId.toStdString() << ",";
	std::cerr << msgId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	requestPulseFocus(groupId, msgId);
}

void WireDialog::PVHviewReply(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHviewReply(";
	std::cerr << groupId.toStdString() << ",";
	std::cerr << msgId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	// requestPulseFocus(groupId, msgId);
}

void WireDialog::PVHfollow(const RsGxsGroupId &groupId)
{
	std::cerr << "WireDialog::PVHfollow(";
	std::cerr << groupId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	RsThread::async([groupId]()
	{
		rsWire->subscribe(groupId, true);
	});
}

void WireDialog::PVHrate(const RsGxsId &authorId)
{
	std::cerr << "WireDialog::PVHrate(";
	std::cerr << authorId.toStdString();
	std::cerr << ") TODO";
	std::cerr << std::endl;
}

void WireDialog::postTestTwitterView()
{
	clearTwitterView();

	addTwitterView(new PulseTopLevel(NULL,RsWirePulseSPtr())); 
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
}

void WireDialog::clearTwitterView()
{
	std::cerr << "WireDialog::clearTwitterView()";
	std::cerr << std::endl;

	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	QLayoutItem *item;
	int i = 0;
	while (i < alayout->count())
	{
		item = alayout->itemAt(i);
		QWidget *widget = item->widget();
		if (NULL != dynamic_cast<PulseViewItem *>(widget))
		{
			std::cerr << "WireDialog::clearTwitterView() Removing Item at: " << i;
			std::cerr << std::endl;

			item = alayout->takeAt(i);
			item->widget()->deleteLater();
			delete item;
		}
		else
		{
			std::cerr << "WireDialog::clearTwitterView() Leaving Item at: " << i;
			std::cerr << std::endl;

			i++;
		}
	}
}

void WireDialog::addTwitterView(PulseViewItem *item)
{
	std::cerr << "WireDialog::addTwitterView()";
	std::cerr << std::endl;

	/* ensure its a boxlayout */
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	QBoxLayout *boxlayout = dynamic_cast<QBoxLayout *>(alayout);
	if (boxlayout == NULL) {
		std::cerr << "WireDialog::addTwitterView() ERROR not boxlayout, not Inserting";
		std::cerr << std::endl;
		return;
	}

	// inserting as last item, but before the trailing spacer (so pulses start at the top).
	std::cerr << "WireDialog::addTwitterView() Inserting at end";
	std::cerr << std::endl;
	int pos = boxlayout->count();
	if (pos > 0 && boxlayout->itemAt(pos - 1)->spacerItem()) {
		--pos;
	}
	boxlayout->insertWidget(pos, item);
}

// HISTORY -------------------------------------------------------------------------------

void printWireViewHistory(const WireViewHistory &view)
{
	std::cerr << "WireViewHistory(" << (int) view.viewType << "): ";
	switch(view.viewType) {
		case WireViewType::PULSE_FOCUS:
			std::cerr << " PULSE_FOCUS: grpId: " << view.groupId;
			std::cerr << " msgId: " << view.msgId;
			std::cerr << std::endl;
			break;
		case WireViewType::GROUP_FOCUS:
			std::cerr << " GROUP_FOCUS: grpId: " << view.groupId;
			std::cerr << std::endl;
			break;
		case WireViewType::GROUPS:
			std::cerr << " GROUPS_PULSES: grpIds: TBD";
			std::cerr << std::endl;
			break;
		default:
			break;
	}
}

void WireDialog::AddToHistory(const WireViewHistory &view)
{
	std::cerr << "AddToHistory():";
	printWireViewHistory(view);

	/* clear future history */
	mHistory.resize(mHistoryIndex + 1);

	mHistory.push_back(view);
	mHistoryIndex = mHistory.size() - 1;

	// at end of history.
	// enable back, disable forward.
	ui.toolButton_back->setEnabled(mHistoryIndex > 0);
	ui.toolButton_forward->setEnabled(false);
}

void WireDialog::back()
{
	LoadHistory(mHistoryIndex-1);
}

void WireDialog::forward()
{
	LoadHistory(mHistoryIndex+1);
}

void WireDialog::LoadHistory(uint32_t index)
{
	if (index >= mHistory.size()) {
		return;
	}

	mHistoryIndex = index;
	WireViewHistory view = mHistory[index];

	std::cerr << "LoadHistory:";
	printWireViewHistory(view);

	switch(view.viewType) {
		case WireViewType::PULSE_FOCUS:
			showPulseFocus(view.groupId, view.msgId);
			break;
		case WireViewType::GROUP_FOCUS:
			showGroupFocus(view.groupId);
			break;
		case WireViewType::GROUPS:
			showGroupsPulses(view.groupIds);
			break;
		default:
			break;
	}

	ui.toolButton_back->setEnabled(index > 0);
	ui.toolButton_forward->setEnabled(index + 1 < mHistory.size());
}
// HISTORY -------------------------------------------------------------------------------

void WireDialog::requestPulseFocus(const RsGxsGroupId groupId, const RsGxsMessageId msgId)
{
	mRestoreScroll = -1;
	WireViewHistory view;
	view.viewType = WireViewType::PULSE_FOCUS;
	view.groupId = groupId;
	view.msgId = msgId;

	AddToHistory(view);
	showPulseFocus(groupId, msgId);
}

void WireDialog::showPulseFocus(const RsGxsGroupId groupId, const RsGxsMessageId msgId)
{
	clearTwitterView();

	// background thread for loading.
	RsThread::async([this, groupId, msgId]()
	{
		// fetch data from backend.
		RsWirePulseSPtr pPulse;
		int type = 0;
		if(rsWire->getPulseFocus(groupId, msgId, type, pPulse))
		{
			// sleep(2);

			/* now insert the pulse + children into the layput */
			RsQThreadUtils::postToObject([pPulse,this]()
			{
				/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

				postPulseFocus(pPulse);

			}, this);
		}
	});
}


void WireDialog::postPulseFocus(RsWirePulseSPtr pPulse)
{
	clearTwitterView();
	if (!pPulse)
	{
		std::cerr << "WireDialog::postPulseFocus() Invalid pulse";
		std::cerr << std::endl;
		return;
	}

	ui.label_viewMode->setText(tr("Pulse"));
	setComposeVisible(false);
	setEmptyFeedMessage(QString());

	PulseTopLevel *topLevel = new PulseTopLevel(this, pPulse);
	addTwitterView(topLevel);
	showReplyBox(topLevel, pPulse);

	for(auto &it : pPulse->mReplies)
	{
		RsWirePulseSPtr reply = it;
		PulseReply *firstReply = new PulseReply(this, reply);
		firstReply->showReferenceHeader(false); // it's obvious in the thread
		addTwitterView(firstReply);

		if (reply->mReplies.size() > 0)
		{
			PulseReply *secondReply = new PulseReply(this, reply->mReplies.front());
			addTwitterView(secondReply);
			firstReply->showReplyLine(true);
			secondReply->showReplyLine(false);
		}
		else
		{
			firstReply->showReplyLine(false);
		}


		if (reply->mReplies.size() > 1)
		{
			// addTwitterView(new PulseMoreReplies(NULL, reply));
		}

		addTwitterView(new PulseReplySeperator());
	}

	// Add big separator, and republishes.
	if (pPulse->mReplies.size() > 0 && pPulse->mRepublishes.size() > 0)
	{
		addTwitterView(new PulseReplySeperator());
		addTwitterView(new PulseReplySeperator());
	}

	for(auto &it : pPulse->mRepublishes)
	{
		RsWirePulseSPtr repub = it;

		PulseReply *firstRepub = new PulseReply(this, repub);
		firstRepub->showReplyLine(false);

		addTwitterView(firstRepub);
		addTwitterView(new PulseReplySeperator());
	}



	restoreScrollPosition();
}

void WireDialog::requestGroupFocus(const RsGxsGroupId groupId)
{
	mRestoreScroll = -1;
	WireViewHistory view;
	view.viewType = WireViewType::GROUP_FOCUS;
	view.groupId = groupId;

	AddToHistory(view);
	showGroupFocus(groupId);
}

void WireDialog::showGroupFocus(const RsGxsGroupId groupId)
{
	clearTwitterView();

	// background thread for loading.
	RsThread::async([this, groupId]()
	{
		// fetch data from backend.
		RsWireGroupSPtr grp;
		std::list<RsWirePulseSPtr> pulses;

		bool success = rsWire->getWireGroup(groupId, grp);
		std::list<RsGxsGroupId> groupIds = { groupId };
		success = rsWire->getPulsesForGroups(groupIds, pulses);

		// sleep(2);

		/* now insert the pulse + children into the layput */
		RsQThreadUtils::postToObject([grp, pulses,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

			postGroupFocus(grp, pulses);

		}, this);
	});
}


void WireDialog::postGroupFocus(RsWireGroupSPtr group, std::list<RsWirePulseSPtr> pulses)
{
    clearTwitterView();

	std::cerr << "WireDialog::postGroupFocus()";
	std::cerr << std::endl;

	if (!group)
	{
		std::cerr << "WireDialog::postGroupFocus() group is INVALID";
		std::cerr << std::endl;
		return;
	}

	ui.label_viewMode->setText(tr("Profile"));
	setComposeVisible(false);
	if (mReplyBox) mReplyBox->hide();
	setEmptyFeedMessage(QString());

	addTwitterView(new PulseViewGroup(this, group));

	for(auto &it : pulses)
	{
		RsWirePulseSPtr reply = it;

		// don't show likes
		if (reply->mPulseType & WIRE_PULSE_TYPE_LIKE) {
			std::cerr << "WireDialog::postGroupFocus() Not showing LIKE";
			std::cerr << std::endl;
			continue;
		}

		PulseReply *firstReply = new PulseReply(this, reply);
		addTwitterView(firstReply);
		firstReply->showReplyLine(false);

		addTwitterView(new PulseReplySeperator());

	}

	restoreScrollPosition();
}

void WireDialog::requestGroupsPulses(const std::list<RsGxsGroupId>& groupIds, bool addToHistory)
{
	if (addToHistory) {
		mRestoreScroll = -1;
	}
	if (addToHistory) {
		WireViewHistory view;
		view.viewType = WireViewType::GROUPS;
		view.groupIds = groupIds;

		AddToHistory(view);
	}
	showGroupsPulses(groupIds, addToHistory);
}

void WireDialog::showGroupsPulses(const std::list<RsGxsGroupId>& groupIds, bool addToHistory)
{
	clearTwitterView();

	// background thread for loading.
	RsThread::async([this, groupIds]()
	{
		// fetch data from backend.
		std::list<RsWirePulseSPtr> pulses;
		if(rsWire->getPulsesForGroups(groupIds, pulses))
		{
			// sleep(2);

			/* now insert the pulse + children into the layput */
			RsQThreadUtils::postToObject([pulses,this]()
			{
				/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

				postGroupsPulses(pulses);

			}, this);
		}
	});
}

void WireDialog::postGroupsPulses(std::list<RsWirePulseSPtr> pulses)
{
    clearTwitterView();
	std::cerr << "WireDialog::postGroupsPulses()";
	std::cerr << std::endl;

	ui.label_viewMode->setText(tr("Home"));
	setComposeVisible(true);
	if (mReplyBox) mReplyBox->hide();
	updateHashtags(pulses);

	// friendly message instead of an empty grey area.
	int shown = 0;
	for (auto &p : pulses) {
		if (p && !(p->mPulseType & WIRE_PULSE_TYPE_LIKE)) {
			++shown;
		}
	}
	if (shown > 0) {
		setEmptyFeedMessage(QString());
	} else if (mGroupSet == GROUP_SET_OWN) {
		setEmptyFeedMessage(tr("You have not posted any pulses yet.\nWrite your first pulse above."));
	} else if (mGroupSet == GROUP_SET_SUBSCRIBED) {
		setEmptyFeedMessage(tr("No pulses from the Wire groups you follow yet.\nFollow more groups on the right, or wait until your friends sync new pulses."));
	} else {
		setEmptyFeedMessage(tr("No pulses received yet.\nPulses appear here when your friends' nodes sync them."));
	}

	for(auto &it : pulses)
	{
		RsWirePulseSPtr reply = it;
		// don't show likes
		if (reply->mPulseType & WIRE_PULSE_TYPE_LIKE) {
			std::cerr << "WireDialog::postGroupsPulses() Not showing LIKE";
			std::cerr << std::endl;
			continue;
		}

		PulseReply *firstReply = new PulseReply(this, reply);
		addTwitterView(firstReply);
		firstReply->showReplyLine(false);

		addTwitterView(new PulseReplySeperator());

	}

	// keep the current search applied after a refresh.
	if (mSearchPulses) {
		filterPulses(mSearchPulses->text());
	}

	restoreScrollPosition();
}

void WireDialog::getServiceStatistics(GxsServiceStatistic& stats) const
{
    if(!mCachedGroupStats.empty())
    {
        stats = GxsServiceStatistic(); // clears everything

        for(auto& it:  mCachedGroupStats)
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

        // Also save the service statistics in conf file, so that we can display it right away at start.

        Settings->beginGroup(mSettingsName);
        Settings->setValue("NumMsgs",            stats.mNumMsgs            );
        Settings->setValue("NumGrps",            stats.mNumGrps            );
        Settings->setValue("SizeOfMessages",     stats.mSizeOfMsgs         );
        Settings->setValue("NumThreadMsgsNew",   stats.mNumThreadMsgsNew   );
        Settings->setValue("NumThreadMsgsUnread",stats.mNumThreadMsgsUnread);
        Settings->setValue("NumChildMsgsNew",    stats.mNumChildMsgsNew    );
        Settings->setValue("NumChildMsgsUnread", stats.mNumChildMsgsUnread );
        Settings->endGroup();
    }
    else	// Get statistics from settings if no cache is already present: allows to display at start.
    {
        Settings->beginGroup(mSettingsName);

        stats.mNumMsgs             = Settings->value("NumMsgs",QVariant(0)).toInt();
        stats.mNumGrps             = Settings->value("NumGrps",QVariant(0)).toInt();
        stats.mSizeOfMsgs          = Settings->value("SizeOfMessages",QVariant(0)).toInt();
        stats.mNumThreadMsgsNew    = Settings->value("NumThreadMsgsNew",QVariant(0)).toInt();
        stats.mNumThreadMsgsUnread = Settings->value("NumThreadMsgsUnread",QVariant(0)).toInt();
        stats.mNumChildMsgsNew     = Settings->value("NumChildMsgsNew",QVariant(0)).toInt();
        stats.mNumChildMsgsUnread  = Settings->value("NumChildMsgsUnread",QVariant(0)).toInt();

        Settings->endGroup();
    }
}

void WireDialog::updateGroupStatistics(const RsGxsGroupId &groupId)
{
    mGroupStatisticsToUpdate.insert(groupId);
    mShouldUpdateGroupStatistics = true;
}

void WireDialog::updateGroupStatisticsReal(const RsGxsGroupId &groupId)
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

            mCachedGroupStats[groupId] = stats;

            // Update the unread count badge on the matching WireGroupItem
            WireGroupItem *groupItem = findGroupItemWidget(groupId);
            if (groupItem)
                groupItem->setUnreadCount(stats.mNumThreadMsgsUnread);

            getUserNotify()->updateIcon();

        }, this );
    });
}

void WireDialog::updateGroupCountLabels()
{
    int allCount = mAllGroups.size();
    int ownCount = mOwnGroups.size();
    
    // Count Subscribed and Others from mAllGroups
    int subscribedCount = 0;
    int othersCount = 0;
    for (auto const& [id, group] : mAllGroups) {
        if (group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) {
            subscribedCount++;
        } else {
            othersCount++;
        }
    }

    // Update ComboBox Text
    ui.comboBox_groupSet->setItemText(GROUP_SET_ALL, tr("All (%1)").arg(allCount));
    ui.comboBox_groupSet->setItemText(GROUP_SET_OWN, tr("Yourself (%1)").arg(ownCount));
    ui.comboBox_groupSet->setItemText(GROUP_SET_SUBSCRIBED, tr("Following (%1)").arg(subscribedCount));
    ui.comboBox_groupSet->setItemText(GROUP_SET_OTHERS, tr("Others (%1)").arg(othersCount));

    if (mFeedTabs) {
        mFeedTabs->setTabText(groupSetToFeedTab(GROUP_SET_SUBSCRIBED), tr("Following"));
        mFeedTabs->setTabText(groupSetToFeedTab(GROUP_SET_ALL), tr("All"));
        mFeedTabs->setTabText(groupSetToFeedTab(GROUP_SET_OWN), tr("Yourself"));
        mFeedTabs->setTabText(groupSetToFeedTab(GROUP_SET_OTHERS), tr("Discover"));
    }
}

bool WireDialog::navigate(const RsGxsGroupId &groupId, const RsGxsMessageId& msgId)
{
	if (groupId.isNull()) {
		return false;
	}

	// if groups not loaded yet, save for later
	if (mAllGroups.empty()) {
		mNavigatePendingGroupId = groupId;
		mNavigatePendingMsgId = msgId;
		return true;
	}

	if (!msgId.isNull()) {
		requestPulseFocus(groupId, msgId);
	} else {
		requestGroupFocus(groupId);
	}

	return true;
}

GxsMessageFrameWidget *WireDialog::messageWidget(const RsGxsGroupId &groupId)
{
//    int tabCount = ui.tabWidget->count();

//    for (int index = 0; index < tabCount; ++index)
//    {
//        GxsMessageFrameWidget *childWidget = dynamic_cast<GxsMessageFrameWidget*>(ui.tabWidget->widget(index));

//        if (childWidget && childWidget->groupId() == groupId)
//            return childWidget;
//    }

    return NULL;
}

WireGroupItem *WireDialog::findGroupItemWidget(const RsGxsGroupId &groupId)
{
    QLayout *alayout = ui.groupsWidget->layout();
    for (int i = 0; i < alayout->count(); ++i)
    {
        QLayoutItem *item = alayout->itemAt(i);
        if (!item) continue;
        WireGroupItem *groupItem = dynamic_cast<WireGroupItem *>(item->widget());
        if (groupItem && groupItem->groupId() == groupId)
            return groupItem;
    }
    return nullptr;
}

//void GxsGroupFrameDialog::changedCurrentGroup(const QString& groupId)
//{
//	if (mInFill) {
//		return;
//	}

//	if (groupId.isEmpty())
//    {
//        auto w = currentWidget();

//        if(w)
//			w->setGroupId(RsGxsGroupId());

//		return;
//	}

//	mGroupId = RsGxsGroupId(groupId.toStdString());

//	if (mGroupId.isNull())
//		return;

//	/* search exisiting tab */
//	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId);

//    // check that we have at least one tab

//	if(msgWidget)
//		ui.messageTabWidget->setCurrentWidget(msgWidget);
//    else
//    {
//        if(useTabs() || ui.messageTabWidget->count()==0)
//        {
//			msgWidget = createMessageWidget(RsGxsGroupId(groupId.toStdString()));
//			ui.messageTabWidget->setCurrentWidget(msgWidget);
//		}
//        else
//			currentWidget()->setGroupId(mGroupId);
//	}
//}

void WireDialog::markGroupAsRead(const RsGxsGroupId &groupId)
{
    // Fetch all message summaries in the background and mark each as read
    // using the blocking API — DB write is complete before each call returns,
    // so the event arrives after the data is committed.
    RsThread::async([this, groupId]()
    {
        std::vector<RsMsgMetaData> summaries;
        if (!rsWire->getContentSummaries(groupId, summaries))
        {
            std::cerr << "WireDialog::markGroupAsRead() failed to get summaries for group "
                      << groupId << std::endl;
            return;
        }

        for (auto &meta : summaries)
        {
            RsGxsGrpMsgIdPair msgIdPair(groupId, meta.mMsgId);
            rsWire->setMessageReadStatus(msgIdPair, true);
        }

        // Stats will be refreshed automatically via the READ_STATUS_CHANGED events
        // fired by setMessageReadStatus for each message.

        requestGroupData();
    });
}


/****************************************************************************************/
// TWITTER-LIKE LAYOUT: feed column, tabs, inline compose box, right sidebar.
/****************************************************************************************/

void WireDialog::setupFeedLayout()
{
	// Feed as one centered column, like Twitter.
	ui.scrollArea->setFrameShape(QFrame::NoFrame);
	ui.scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	ui.scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui.scrollAreaWidgetContents->setMaximumWidth(WIRE_FEED_WIDTH);

	// transparent feed background: "trans_" names use the transparent rule of the qss.
	ui.scrollArea->setObjectName("trans_wireFeedScrollArea");

	// title of the view ("Home", "Profile", "Pulse")
	QFont titleFont = ui.label_viewMode->font();
	titleFont.setBold(true);
	titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
	ui.label_viewMode->setFont(titleFont);
	ui.label_viewMode->setText(tr("Home"));

	// The spacer used to be the first item, which pushed short feeds to the bottom.
	// Move it to the end: pulses are inserted before it (see addTwitterView).
	QVBoxLayout *feedLayout = ui.verticalLayout_2;
	for (int i = 0; i < feedLayout->count(); ++i) {
		QLayoutItem *item = feedLayout->itemAt(i);
		if (item && item->spacerItem()) {
			feedLayout->takeAt(i);
			feedLayout->addItem(item);
			break;
		}
	}

	// message shown when a feed has no pulses (stays at the top of the feed).
	mEmptyFeedLabel = new QLabel(ui.scrollAreaWidgetContents);
	mEmptyFeedLabel->setObjectName("wireMutedLabel");
	mEmptyFeedLabel->setAlignment(Qt::AlignCenter);
	mEmptyFeedLabel->setWordWrap(true);
	mEmptyFeedLabel->setContentsMargins(24, 40, 24, 40);
	mEmptyFeedLabel->hide();
	feedLayout->insertWidget(0, mEmptyFeedLabel);

	// the left account list had the same problem (spacer first).
	QBoxLayout *groupsLayout = dynamic_cast<QBoxLayout *>(ui.groupsWidget->layout());
	if (groupsLayout) {
		for (int i = 0; i < groupsLayout->count(); ++i) {
			QLayoutItem *item = groupsLayout->itemAt(i);
			if (item && item->spacerItem()) {
				groupsLayout->takeAt(i);
				groupsLayout->addItem(item);
				break;
			}
		}
	}

	// Header above the feed: tabs + compose box, centered with the feed.
	mFeedHeader = new QWidget(this);
	mFeedHeader->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum); // never grow taller than needed
	QHBoxLayout *centerLayout = new QHBoxLayout(mFeedHeader);
	centerLayout->setContentsMargins(0, 0, 0, 0);

	QWidget *column = new QWidget(mFeedHeader);
	column->setMaximumWidth(WIRE_FEED_WIDTH);
	column->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	QVBoxLayout *columnLayout = new QVBoxLayout(column);
	columnLayout->setContentsMargins(0, 0, 0, 0);
	columnLayout->setSpacing(0);

	mFeedTabs = new QTabBar(column);
	mFeedTabs->setObjectName("wireFeedTabs");
	mFeedTabs->setExpanding(true);
	mFeedTabs->setDrawBase(false);
	mFeedTabs->setDocumentMode(true);
	for (int i = 0; i < sFeedTabCount; ++i) {
		mFeedTabs->addTab(QString());
	}
	mFeedTabs->setCurrentIndex(groupSetToFeedTab(mGroupSet));
	connect(mFeedTabs, SIGNAL(currentChanged(int)), this, SLOT(selectFeedTab(int)));
	columnLayout->addWidget(mFeedTabs);

	centerLayout->addStretch(1);
	centerLayout->addWidget(column, 100);
	centerLayout->addStretch(1);

	// put the header between the range bar and the feed.
	int idx = ui.verticalLayout_3->indexOf(ui.scrollArea);
	ui.verticalLayout_3->insertWidget(idx < 0 ? 0 : idx, mFeedHeader);
	ui.verticalLayout_3->setStretchFactor(mFeedHeader, 0);
	ui.verticalLayout_3->setStretchFactor(ui.scrollArea, 1);
	ui.verticalLayout_3->setSpacing(0);

	updateGroupCountLabels(); // sets the tab texts
}

void WireDialog::setupComposeBox()
{
	QWidget *column = mFeedTabs ? mFeedTabs->parentWidget() : nullptr;
	if (!column || !column->layout()) {
		return;
	}

	QFrame *box = new QFrame(column);
	box->setObjectName("wireComposeBox");
	box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	mComposeBox = box;

	QHBoxLayout *boxLayout = new QHBoxLayout(box);
	boxLayout->setContentsMargins(12, 10, 12, 10);
	boxLayout->setSpacing(10);

	mComposeAvatar = new QLabel(box);
	mComposeAvatar->setObjectName("wireAvatar"); // no qss padding, else the picture is cut
	mComposeAvatar->setFixedSize(48, 48);
	mComposeAvatar->setAlignment(Qt::AlignTop);
	boxLayout->addWidget(mComposeAvatar, 0, Qt::AlignTop);

	QVBoxLayout *right = new QVBoxLayout();
	right->setSpacing(6);
	right->setContentsMargins(0, 0, 0, 0);
	boxLayout->addLayout(right, 1);

	mComposeEdit = new QTextEdit(box);
	mComposeEdit->setObjectName("wireComposeEdit");
	mComposeEdit->setAcceptRichText(false);
	mComposeEdit->setPlaceholderText(tr("What's happening?"));
	mComposeEdit->setFrameShape(QFrame::NoFrame);
	mComposeEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mComposeEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QFont f = mComposeEdit->font();
	f.setPointSizeF(f.pointSizeF() * 1.3);
	mComposeEdit->setFont(f);
	mComposeEdit->setFixedHeight(mComposeEdit->fontMetrics().lineSpacing() * 2 + 16);
	connect(mComposeEdit, SIGNAL(textChanged()), this, SLOT(composeTextChanged()));
	right->addWidget(mComposeEdit);

	QHBoxLayout *bar = new QHBoxLayout();
	bar->setSpacing(6);
	right->addLayout(bar);

	// "Post as": reuse the account chooser from the toolbar.
	ui.toolBarFrame_HL->removeWidget(ui.groupChooser);
	ui.groupChooser->setParent(box);
	ui.groupChooser->setToolTip(tr("Post as"));
	ui.groupChooser->setIconSize(QSize(20, 20));
	ui.groupChooser->setSizeAdjustPolicy(QComboBox::AdjustToContents); // show the whole account name
	bar->addWidget(ui.groupChooser);
	connect(ui.groupChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(composeAccountChanged(int)));

	QToolButton *moreButton = new QToolButton(box);
	moreButton->setObjectName("wireComposeImages");
	moreButton->setText(tr("Images"));
	moreButton->setToolTip(tr("Open the full editor to add images"));
	WireStyleActionButton(moreButton);
	moreButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/compose.png"));
	connect(moreButton, SIGNAL(clicked()), this, SLOT(createPulse()));
	bar->addWidget(moreButton);

	bar->addStretch(1);

	mComposePost = new QPushButton(tr("Post"), box);
	mComposePost->setObjectName("postButton"); // existing blue button style of default.qss
	setPostButtonFont(mComposePost);
	mComposePost->setCursor(Qt::PointingHandCursor);
	mComposePost->setEnabled(false);
	connect(mComposePost, SIGNAL(clicked()), this, SLOT(postInlinePulse()));
	bar->addWidget(mComposePost);

	column->layout()->addWidget(box);
}

void WireDialog::setComposeVisible(bool visible)
{
	if (mComposeBox) {
		mComposeBox->setVisible(visible);
	}
}

void WireDialog::composeAccountChanged(int index)
{
	if (!mComposeAvatar) {
		return;
	}

	QPixmap pixmap;
	if (index >= 0 && index < (int)mOwnGroups.size())
	{
		const RsWireGroup &group = mOwnGroups[index];
		if (group.mHeadshot.mData) {
			GxsIdDetails::loadPixmapFromData(group.mHeadshot.mData, group.mHeadshot.mSize, pixmap, GxsIdDetails::ORIGINAL);
		}
		if (pixmap.isNull()) {
			pixmap = GxsIdDetails::makeDefaultGroupIcon(group.mMeta.mGroupId, IMAGE_WIRE, GxsIdDetails::ORIGINAL);
		}
	}
	else
	{
		pixmap = FilesDefs::getPixmapFromQtResourcePath(IMAGE_WIRE);
	}
	mComposeAvatar->setPixmap(WireRoundPixmap(pixmap, 48));
	if (mReplyAvatar) {
		mReplyAvatar->setPixmap(WireRoundPixmap(pixmap, 40));
	}
	replyTextChanged();

	if (mComposeEdit) {
		mComposeEdit->setPlaceholderText(mOwnGroups.empty()
			? tr("Create a Wire account to post pulses")
			: tr("What's happening?"));
	}
	composeTextChanged();
}

void WireDialog::composeTextChanged()
{
	if (!mComposeEdit || !mComposePost) {
		return;
	}

	bool hasText = !mComposeEdit->toPlainText().trimmed().isEmpty();
	mComposePost->setEnabled(hasText && !mOwnGroups.empty());

	// grow with the text: 2 lines minimum, 6 lines maximum (then scroll).
	int lineHeight = mComposeEdit->fontMetrics().lineSpacing();
	int minHeight = lineHeight * 2 + 16;
	int maxHeight = lineHeight * 6 + 16;
	int height = minHeight;
	if (mComposeEdit->viewport()->width() > 50 && hasText) {
		height = (int) mComposeEdit->document()->size().height() + 16;
	}
	mComposeEdit->setFixedHeight(qBound(minHeight, height, maxHeight));
}

void WireDialog::postInlinePulse()
{
	if (!mComposeEdit) {
		return;
	}

	QString text = mComposeEdit->toPlainText().trimmed();
	if (text.isEmpty()) {
		return;
	}

	int idx = ui.groupChooser->currentIndex();
	if (idx < 0 || idx >= (int)mOwnGroups.size()) {
		QMessageBox::warning(this, tr("RetroShare"), tr("Please create or choose Wire Groupd first"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	RsGxsGroupId groupId = mOwnGroups[idx].mMeta.mGroupId;

	RsWirePulseSPtr pPulse(new RsWirePulse());
	pPulse->mSentiment = WIRE_PULSE_SENTIMENT_NO_SENTIMENT;
	pPulse->mPulseText = text.toStdString();

	mComposePost->setEnabled(false);

	RsThread::async([this, groupId, pPulse]()
	{
		bool ok = rsWire->createOriginalPulse(groupId, pPulse);

		RsQThreadUtils::postToObject([this, ok]()
		{
			if (ok) {
				mComposeEdit->clear();
			} else {
				QMessageBox::warning(this, tr("RetroShare"), tr("Could not post the pulse."), QMessageBox::Ok, QMessageBox::Ok);
			}
			composeTextChanged();
		}, this);
	});
}

void WireDialog::selectFeedTab(int index)
{
	if (index < 0 || index >= sFeedTabCount) {
		return;
	}

	// back to the feed when a tab is clicked in a focus view.
	mGroupSelected = nullptr;

	int groupSet = sFeedTabGroupSets[index];
	if (ui.comboBox_groupSet->currentIndex() != groupSet) {
		ui.comboBox_groupSet->setCurrentIndex(groupSet); // calls selectGroupSet()
	} else {
		selectGroupSet(groupSet);
	}
}

void WireDialog::setupSidebar()
{
	mSidebar = new QScrollArea(ui.splitter);
	mSidebar->setObjectName("trans_wireSidebar");
	mSidebar->setWidgetResizable(true);
	mSidebar->setFrameShape(QFrame::NoFrame);
	mSidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mSidebar->setMinimumWidth(260);
	mSidebar->setMaximumWidth(360);

	QWidget *content = new QWidget(mSidebar);
	QVBoxLayout *layout = new QVBoxLayout(content);
	layout->setContentsMargins(12, 8, 12, 12);
	layout->setSpacing(12);

	// Search (local: filters the loaded pulses).
	mSearchPulses = new QLineEdit(content);
	mSearchPulses->setObjectName("wireSearchPulses");
	mSearchPulses->setPlaceholderText(tr("Search pulses"));
	mSearchPulses->setClearButtonEnabled(true);
	connect(mSearchPulses, SIGNAL(textChanged(const QString&)), this, SLOT(filterPulses(const QString&)));
	layout->addWidget(mSearchPulses);

	// Hashtags in the loaded feed (Wire has no server, so no global trends).
	layout->addWidget(createSidebarCard(tr("Hashtags in your feed"), &mHashtagsLayout));

	// Wire groups you don't follow yet.
	QFrame *followCard = createSidebarCard(tr("Wire groups to follow"), &mFollowLayout);
	QPushButton *showMore = new QPushButton(tr("Show more"), followCard);
	showMore->setObjectName("wireLinkButton");
	showMore->setFlat(true);
	showMore->setCursor(Qt::PointingHandCursor);
	connect(showMore, SIGNAL(clicked()), this, SLOT(showOtherGroups()));
	followCard->layout()->addWidget(showMore);
	layout->addWidget(followCard);

	// Network status.
	QVBoxLayout *networkLayout = nullptr;
	layout->addWidget(createSidebarCard(tr("Network"), &networkLayout));
	mNetworkOnline = new QLabel(content);
	mNetworkNewest = new QLabel(content);
	mNetworkNewest->setObjectName("wireMutedLabel");
	networkLayout->addWidget(mNetworkOnline);
	networkLayout->addWidget(mNetworkNewest);

	QLabel *footer = new QLabel(tr("RetroShare Wire - decentralised pulses"), content);
	footer->setObjectName("wireMutedLabel");
	footer->setWordWrap(true);
	layout->addWidget(footer);

	layout->addStretch(1);
	mSidebar->setWidget(content);

	ui.splitter->addWidget(mSidebar);
	ui.splitter->setStretchFactor(0, 0);
	ui.splitter->setStretchFactor(1, 1);
	ui.splitter->setStretchFactor(2, 0);

	mNetworkTimer = new QTimer(this);
	connect(mNetworkTimer, SIGNAL(timeout()), this, SLOT(updateNetworkCard()));
	mNetworkTimer->start(30 * 1000);

	updateHashtags(std::list<RsWirePulseSPtr>());
	updateGroupsToFollow();
	updateNetworkCard();
}

void WireDialog::resizeEvent(QResizeEvent *event)
{
	MainPage::resizeEvent(event);
	if (mSidebar) {
		mSidebar->setVisible(width() >= WIRE_SIDEBAR_MIN_WINDOW);
	}
}

void WireDialog::filterPulses(const QString &text)
{
	QString filter = text.trimmed();
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	for (int i = 0; i < alayout->count(); ++i)
	{
		PulseDataItem *item = dynamic_cast<PulseDataItem *>(alayout->itemAt(i)->widget());
		if (item) {
			item->setVisible(item->matchesFilter(filter));
		}
	}
}

void WireDialog::updateHashtags(const std::list<RsWirePulseSPtr> &pulses)
{
	if (!mHashtagsLayout) {
		return;
	}
	clearWireLayout(mHashtagsLayout);

	// count each tag once per pulse.
	QMap<QString, int> counts;
	QRegularExpression re("#(\\w+)", QRegularExpression::UseUnicodePropertiesOption);
	for (auto &pulse : pulses)
	{
		if (!pulse || (pulse->mPulseType & WIRE_PULSE_TYPE_LIKE)) {
			continue;
		}

		QString text = QString::fromStdString(pulse->mPulseText);
		if (pulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
			text += " " + QString::fromStdString(pulse->mRefPulseText);
		}

		QSet<QString> tags;
		QRegularExpressionMatchIterator it = re.globalMatch(text);
		while (it.hasNext()) {
			tags.insert("#" + it.next().captured(1).toLower());
		}
		for (const QString &tag : tags) {
			counts[tag]++;
		}
	}

	QList<QPair<int, QString> > sorted;
	for (auto it = counts.begin(); it != counts.end(); ++it) {
		sorted.append(qMakePair(it.value(), it.key()));
	}
	std::sort(sorted.begin(), sorted.end(), [](const QPair<int, QString> &a, const QPair<int, QString> &b) {
		return (a.first != b.first) ? (a.first > b.first) : (a.second < b.second);
	});

	if (sorted.isEmpty())
	{
		QLabel *empty = new QLabel(tr("No hashtags in your feed yet."));
		empty->setObjectName("wireMutedLabel");
		mHashtagsLayout->addWidget(empty);
		return;
	}

	for (int i = 0; i < sorted.size() && i < 5; ++i)
	{
		QString tag = sorted[i].second;
		QPushButton *button = new QPushButton(QString("%1   %2").arg(tag, tr("%n pulse(s)", "", sorted[i].first)));
		button->setObjectName("wireHashtagButton");
		button->setFlat(true);
		button->setCursor(Qt::PointingHandCursor);
		button->setToolTip(tr("Show pulses with %1").arg(tag));
		connect(button, &QPushButton::clicked, this, [this, tag]() {
			if (mSearchPulses) {
				mSearchPulses->setText(tag);
			}
		});
		mHashtagsLayout->addWidget(button);
	}
}

void WireDialog::updateGroupsToFollow()
{
	if (!mFollowLayout) {
		return;
	}
	clearWireLayout(mFollowLayout);

	std::vector<RsWireGroup> candidates;
	for (auto &it : mAllGroups)
	{
		uint32_t flags = it.second.mMeta.mSubscribeFlags;
		if (flags & (GXS_SERV::GROUP_SUBSCRIBE_ADMIN | GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)) {
			continue;
		}
		candidates.push_back(it.second);
	}

	// most recently active first.
	std::sort(candidates.begin(), candidates.end(), [](const RsWireGroup &a, const RsWireGroup &b) {
		return a.mMeta.mLastPost > b.mMeta.mLastPost;
	});

	if (candidates.empty())
	{
		QLabel *empty = new QLabel(tr("You already follow every Wire group you know."));
		empty->setObjectName("wireMutedLabel");
		empty->setWordWrap(true);
		mFollowLayout->addWidget(empty);
		return;
	}

	for (size_t i = 0; i < candidates.size() && i < 3; ++i)
	{
		const RsWireGroup &group = candidates[i];
		RsGxsGroupId groupId = group.mMeta.mGroupId;

		QWidget *row = new QWidget();
		row->setObjectName("trans_wireFollowRow");
		QHBoxLayout *rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 4, 0, 4);
		rowLayout->setSpacing(8);

		QPixmap pixmap;
		if (group.mHeadshot.mData) {
			GxsIdDetails::loadPixmapFromData(group.mHeadshot.mData, group.mHeadshot.mSize, pixmap, GxsIdDetails::ORIGINAL);
		}
		if (pixmap.isNull()) {
			pixmap = GxsIdDetails::makeDefaultGroupIcon(groupId, IMAGE_WIRE, GxsIdDetails::ORIGINAL);
		}
		QLabel *avatar = new QLabel(row);
		avatar->setObjectName("wireAvatar");
		avatar->setFixedSize(36, 36);
		avatar->setPixmap(WireRoundPixmap(pixmap, 36));
		rowLayout->addWidget(avatar);

		QString subtitle = group.mTagline.empty()
			? WireShortId(groupId.toStdString())
			: QString::fromStdString(group.mTagline);
		QWidget *names = new QWidget(row);
		names->setObjectName("trans_wireFollowNames");
		names->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		names->setToolTip(QString::fromStdString(groupId.toStdString()));
		QVBoxLayout *namesLayout = new QVBoxLayout(names);
		namesLayout->setContentsMargins(0, 0, 0, 0);
		namesLayout->setSpacing(0);

		QLabel *name = new QLabel(QString::fromStdString(group.mMeta.mGroupName), names);
		name->setTextFormat(Qt::PlainText);
		QFont nameFont = name->font();
		nameFont.setBold(true);
		name->setFont(nameFont);
		namesLayout->addWidget(name);

		QLabel *sub = new QLabel(subtitle, names);
		sub->setObjectName("wireMutedLabel");
		sub->setTextFormat(Qt::PlainText);
		namesLayout->addWidget(sub);

		rowLayout->addWidget(names, 1);

		QToolButton *follow = new QToolButton(row);
		follow->setText(tr("Follow"));
		follow->setObjectName("followButton"); // existing follow button style of default.qss
		WireFitButtonText(follow);
		follow->setCursor(Qt::PointingHandCursor);
		connect(follow, &QToolButton::clicked, this, [this, groupId, follow]() {
			RsGxsGroupId id = groupId;
			subscribe(id);
			follow->setText(tr("Following"));
			WireFitButtonText(follow);
			follow->setEnabled(false);
		});
		rowLayout->addWidget(follow);

		mFollowLayout->addWidget(row);
	}
}

void WireDialog::showOtherGroups()
{
	if (mFeedTabs) {
		mFeedTabs->setCurrentIndex(groupSetToFeedTab(GROUP_SET_OTHERS));
	}
}

void WireDialog::updateNetworkCard()
{
	if (!mNetworkOnline || !mNetworkNewest) {
		return;
	}

	std::list<RsPeerId> online;
	if (rsPeers) {
		rsPeers->getOnlineList(online);
	}
	mNetworkOnline->setText(tr("%n friend(s) online", "", (int) online.size()));

	// newest pulse from groups we follow (not our own).
	rstime_t newest = 0;
	for (auto &it : mAllGroups)
	{
		uint32_t flags = it.second.mMeta.mSubscribeFlags;
		if ((flags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) && !(flags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)) {
			newest = std::max(newest, (rstime_t) it.second.mMeta.mLastPost);
		}
	}

	mNetworkNewest->setText(newest > 0
		? tr("Newest pulse received: %1").arg(WireShortTime(newest))
		: tr("No pulses received yet"));
}


void WireDialog::setEmptyFeedMessage(const QString &text)
{
	if (!mEmptyFeedLabel) {
		return;
	}
	mEmptyFeedLabel->setText(text);
	mEmptyFeedLabel->setVisible(!text.isEmpty());
}

// Left column like Twitter: navigation on top, then the account list.
void WireDialog::setupLeftNavigation()
{
	// Compact navigation: one row [Home] [Profile] ........ [+], then "Post Pulse".
	QWidget *nav = new QWidget(ui.plainFrame_GroupSet);
	nav->setObjectName("trans_wireNav");
	QVBoxLayout *navLayout = new QVBoxLayout(nav);
	navLayout->setContentsMargins(0, 2, 0, 4);
	navLayout->setSpacing(4);

	QHBoxLayout *row = new QHBoxLayout();
	row->setContentsMargins(0, 0, 0, 0);
	row->setSpacing(2);
	navLayout->addLayout(row);

	auto addNavButton = [&](const QIcon &icon, const QString &text, const QString &tooltip, const char *slot) {
		QToolButton *button = new QToolButton(nav);
		button->setObjectName("wireNavButton");
		button->setIcon(icon);
		button->setIconSize(QSize(20, 20));
		button->setText(text);
		button->setToolTip(tooltip);
		button->setToolButtonStyle(text.isEmpty() ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
		button->setAutoRaise(true);
		button->setCursor(Qt::PointingHandCursor);
		connect(button, SIGNAL(clicked()), this, slot);
		row->addWidget(button);
		return button;
	};

	addNavButton(ui.toolButton_home->icon(), tr("Home"), tr("Show the pulses of the groups you follow"), SLOT(showHomeFeed()));
	addNavButton(FilesDefs::getIconFromQtResourcePath(":/icons/png/wire-circle.png"), tr("Profile"), tr("Show your profile"), SLOT(showOwnProfile()));
	row->addStretch(1);
	addNavButton(ui.toolButton_createAccount->icon(), QString(), tr("Create account"), SLOT(createGroup()));

	QPushButton *postButton = new QPushButton(tr("Post Pulse"), nav);
	postButton->setObjectName("postButton"); // existing blue button style of default.qss
	setPostButtonFont(postButton);
	postButton->setCursor(Qt::PointingHandCursor);
	connect(postButton, SIGNAL(clicked()), this, SLOT(createPulse()));
	navLayout->addWidget(postButton);

	QFrame *line = new QFrame(nav);
	line->setObjectName("wireNavLine");
	line->setFrameShape(QFrame::HLine);
	navLayout->addWidget(line);

	ui.plainFrame_GroupSet_VL->insertWidget(0, nav);

	// The old toolbar is replaced by this navigation, the group set combo by the feed tabs.
	ui.toolBarFrame->hide();
	ui.comboBox_groupSet->hide();
	ui.lineEdit_searchUsers->setPlaceholderText(tr("Search accounts"));
}

void WireDialog::showOwnProfile()
{
	int idx = ui.groupChooser->currentIndex();
	if (idx < 0 || idx >= (int)mOwnGroups.size()) {
		QMessageBox::warning(this, tr("RetroShare"), tr("Please create or choose Wire Groupd first"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}
	requestGroupFocus(mOwnGroups[idx].mMeta.mGroupId);
}


// Inline "Post your reply" box shown below the pulse in the thread view (like Twitter).
void WireDialog::setupReplyBox()
{
	QFrame *box = new QFrame(ui.scrollAreaWidgetContents);
	box->setObjectName("wireComposeBox"); // same look as the compose box
	box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	mReplyBox = box;

	QHBoxLayout *layout = new QHBoxLayout(box);
	layout->setContentsMargins(12, 8, 12, 8);
	layout->setSpacing(10);

	mReplyAvatar = new QLabel(box);
	mReplyAvatar->setObjectName("wireAvatar");
	mReplyAvatar->setFixedSize(40, 40);
	layout->addWidget(mReplyAvatar, 0, Qt::AlignTop);

	mReplyEdit = new QTextEdit(box);
	mReplyEdit->setObjectName("wireComposeEdit");
	mReplyEdit->setAcceptRichText(false);
	mReplyEdit->setPlaceholderText(tr("Post your reply"));
	mReplyEdit->setFrameShape(QFrame::NoFrame);
	mReplyEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QFont f = mReplyEdit->font();
	f.setPointSizeF(f.pointSizeF() * 1.3);
	mReplyEdit->setFont(f);
	mReplyEdit->setFixedHeight(mReplyEdit->fontMetrics().lineSpacing() + 16);
	connect(mReplyEdit, SIGNAL(textChanged()), this, SLOT(replyTextChanged()));
	layout->addWidget(mReplyEdit, 1);

	mReplyPost = new QPushButton(tr("Reply"), box);
	mReplyPost->setObjectName("postButton"); // existing blue button style of default.qss
	setPostButtonFont(mReplyPost);
	mReplyPost->setCursor(Qt::PointingHandCursor);
	mReplyPost->setEnabled(false);
	connect(mReplyPost, SIGNAL(clicked()), this, SLOT(postInlineReply()));
	layout->addWidget(mReplyPost, 0, Qt::AlignTop);

	box->hide();
}

void WireDialog::showReplyBox(PulseViewItem *after, RsWirePulseSPtr pulse)
{
	if (!mReplyBox || !pulse) {
		return;
	}

	// a REF pulse can't be replied to (same rule as PulseDataItem::actionReply),
	// and libretroshare only accepts replies to groups we follow (or own).
	bool followed = pulse->mGroupPtr && (pulse->mGroupPtr->mMeta.mSubscribeFlags &
		(GXS_SERV::GROUP_SUBSCRIBE_ADMIN | GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED));
	if ((pulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) || !followed) {
		mReplyBox->hide();
		return;
	}

	mReplyToGroupId = pulse->mMeta.mGroupId;
	mReplyToMsgId = pulse->mMeta.mOrigMsgId;
	if (mReplyToMsgId.isNull()) {
		mReplyToMsgId = pulse->mMeta.mMsgId;
	}

	// place it right below the pulse.
	QBoxLayout *layout = ui.verticalLayout_2;
	layout->removeWidget(mReplyBox);
	int idx = layout->indexOf(after);
	layout->insertWidget(idx < 0 ? 0 : idx + 1, mReplyBox);
	mReplyBox->show();
	replyTextChanged();
}

void WireDialog::replyTextChanged()
{
	if (!mReplyEdit || !mReplyPost) {
		return;
	}

	bool hasText = !mReplyEdit->toPlainText().trimmed().isEmpty();
	mReplyPost->setEnabled(hasText && !mOwnGroups.empty());

	// one line when empty, grows up to 6 lines.
	int lineHeight = mReplyEdit->fontMetrics().lineSpacing();
	int height = lineHeight + 16;
	if (hasText && mReplyEdit->viewport()->width() > 50) {
		height = (int) mReplyEdit->document()->size().height() + 16;
	}
	mReplyEdit->setFixedHeight(qBound(lineHeight + 16, height, lineHeight * 6 + 16));
}

void WireDialog::postInlineReply()
{
	if (!mReplyEdit) {
		return;
	}

	QString text = mReplyEdit->toPlainText().trimmed();
	if (text.isEmpty() || mReplyToGroupId.isNull() || mReplyToMsgId.isNull()) {
		return;
	}

	int idx = ui.groupChooser->currentIndex();
	if (idx < 0 || idx >= (int)mOwnGroups.size()) {
		QMessageBox::warning(this, tr("RetroShare"), tr("Please create or choose Wire Groupd first"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	RsGxsGroupId replyWith = mOwnGroups[idx].mMeta.mGroupId;
	RsGxsGroupId parentGroupId = mReplyToGroupId;
	RsGxsMessageId parentMsgId = mReplyToMsgId;

	RsWirePulseSPtr pPulse(new RsWirePulse());
	pPulse->mSentiment = WIRE_PULSE_SENTIMENT_NO_SENTIMENT;
	pPulse->mPulseText = text.toStdString();

	mReplyPost->setEnabled(false);

	RsThread::async([this, parentGroupId, parentMsgId, replyWith, pPulse]()
	{
		bool ok = rsWire->createReplyPulse(parentGroupId, parentMsgId, replyWith, WIRE_PULSE_TYPE_REPLY, pPulse);

		RsQThreadUtils::postToObject([this, ok]()
		{
			if (ok) {
				mReplyEdit->clear();
			} else {
				QMessageBox::warning(this, tr("RetroShare"),
					tr("Could not post the reply.\nYou can only reply to pulses of Wire groups you follow."),
					QMessageBox::Ok, QMessageBox::Ok);
			}
			replyTextChanged();
		}, this);
	});
}

// after a background refresh, go back to where the user was.
void WireDialog::restoreScrollPosition()
{
	if (mRestoreScroll < 0) {
		return;
	}
	int value = mRestoreScroll;
	mRestoreScroll = -1;

	// the layout is updated asynchronously, wait for the new size.
	QTimer::singleShot(0, this, [this, value]() {
		ui.scrollArea->verticalScrollBar()->setValue(value);
	});
}
