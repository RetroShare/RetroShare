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

#include <QMenu>
#include <QMessageBox>

#include "GxsForumsDialog.h"
#include "GxsForumGroupDialog.h"
#include "GxsForumThreadWidget.h"

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/channels/ShareKey.h"
#include "gui/common/RSTreeWidget.h"
#include "gui/notifyqt.h"
//#include "gui/common/UIStateHelper.h"

// These should be in retroshare/ folder.
#include "retroshare/rsgxsflags.h"

//#define DEBUG_FORUMS

/* Images for TreeWidget */
#define IMAGE_FOLDER         ":/images/folder16.png"
#define IMAGE_FOLDERGREEN    ":/images/folder_green.png"
#define IMAGE_FOLDERRED      ":/images/folder_red.png"
#define IMAGE_FOLDERYELLOW   ":/images/folder_yellow.png"
#define IMAGE_FORUM          ":/images/konversation.png"
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
#define IMAGE_NEWFORUM       ":/images/new_forum16.png"
#define IMAGE_FORUMAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"

#define TOKEN_TYPE_LISTING          1
#define TOKEN_TYPE_SUBSCRIBE_CHANGE 2
//#define TOKEN_TYPE_CURRENTFORUM   3

/*
 * Transformation Notes:
 *   there are still a couple of things that the new forums differ from Old version.
 *   these will need to be addressed in the future.
 *     -> Child TS (for sorting) is not handled by GXS, this will probably have to be done in the GUI.
 *     -> Need to handle IDs properly.
 *     -> Popularity not handled in GXS yet.
 *     -> Much more to do.
 */

/** Constructor */
GxsForumsDialog::GxsForumsDialog(QWidget *parent)
: RsGxsUpdateBroadcastPage(rsGxsForums, parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	/* Setup Queue */
	mForumQueue = new TokenQueue(rsGxsForums->getTokenService(), this);
	mThreadWidget = NULL;

	/* Setup UI helper */
//	mStateHelper = new UIStateHelper(this);
	// no widget to add yet

	connect(ui.forumTreeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(forumListCustomPopupMenu(QPoint)));
	connect(ui.newForumButton, SIGNAL(clicked()), this, SLOT(newforum()));
	connect(ui.forumTreeWidget, SIGNAL(treeItemActivated(QString)), this, SLOT(changedForum(QString)));
	connect(ui.forumTreeWidget->treeWidget(), SIGNAL(signalMouseMiddleButtonClicked(QTreeWidgetItem*)), this, SLOT(forumTreeMiddleButtonClicked(QTreeWidgetItem*)));
	connect(ui.threadTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(threadTabCloseRequested(int)));
	connect(ui.threadTabWidget, SIGNAL(currentChanged(int)), this, SLOT(threadTabChanged(int)));
	connect(NotifyQt::getInstance(), SIGNAL(forumMsgReadSatusChanged(QString,QString,int)), this, SLOT(forumMsgReadSatusChanged(QString,QString,int)));
	connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	connect(ui.todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));

	/* Initialize group tree */
	ui.forumTreeWidget->initDisplayMenu(ui.displayButton);

	/* Set initial size the splitter */
	QList<int> sizes;
	sizes << 300 << width(); // Qt calculates the right sizes
	ui.splitter->setSizes(sizes);

	/* create forum tree */
	yourForums = ui.forumTreeWidget->addCategoryItem(tr("My Forums"), QIcon(IMAGE_FOLDER), true);
	subscribedForums = ui.forumTreeWidget->addCategoryItem(tr("Subscribed Forums"), QIcon(IMAGE_FOLDERRED), true);
	popularForums = ui.forumTreeWidget->addCategoryItem(tr("Popular Forums"), QIcon(IMAGE_FOLDERGREEN), false);
	otherForums = ui.forumTreeWidget->addCategoryItem(tr("Other Forums"), QIcon(IMAGE_FOLDERYELLOW), false);

	// load settings
	processSettings(true);

	settingsChanged();
}

GxsForumsDialog::~GxsForumsDialog()
{
	// save settings
	processSettings(false);

	delete(mForumQueue);
}

//UserNotify *GxsForumsDialog::getUserNotify(QObject *parent)
//{
//	return new GxsForumUserNotify(parent);
//}

void GxsForumsDialog::todo()
{
	QMessageBox::information(this, "Todo",
							 "<b>Open points:</b><ul>"
							 "<li>Restore forum keys"
							 "<li>Display AUTHD"
							 "<li>Copy/navigate forum link"
							 "<li>Display count of unread messages"
							 "<li>Show/Edit forum details"
							 "<li>Don't show own posts as unread"
							 "<li>Remove messages"
							 "</ul>");
}

void GxsForumsDialog::processSettings(bool load)
{
	Settings->beginGroup(QString("GxsForumsDialog"));

	if (load) {
		// load settings

		// state of splitter
		ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
	} else {
		// save settings

		// state of splitter
		Settings->setValue("Splitter", ui.splitter->saveState());
	}

	ui.forumTreeWidget->processSettings(Settings, load);

	Settings->endGroup();
}

void GxsForumsDialog::settingsChanged()
{
	if (Settings->getForumOpenAllInNewTab()) {
		if (mThreadWidget) {
			delete(mThreadWidget);
			mThreadWidget = NULL;
		}
	} else {
		if (!mThreadWidget) {
			mThreadWidget = createThreadWidget("");
			// remove close button of the the first tab
			ui.threadTabWidget->hideCloseButton(ui.threadTabWidget->indexOf(mThreadWidget));
		}
	}
}

void GxsForumsDialog::forumListCustomPopupMenu(QPoint /*point*/)
{
	int subscribeFlags = ui.forumTreeWidget->subscribeFlags(QString::fromStdString(mForumId));

	QMenu contextMnu(this);

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::forumListCustomPopupMenu()";
	std::cerr << std::endl;
	std::cerr << "    mForumId: " << mForumId;
	std::cerr << std::endl;
	std::cerr << "    subscribeFlags: " << subscribeFlags;
	std::cerr << std::endl;
	std::cerr << "    IS_GROUP_SUBSCRIBED(): " << IS_GROUP_SUBSCRIBED(subscribeFlags);
	std::cerr << std::endl;
	std::cerr << "    IS_GROUP_ADMIN(): " << IS_GROUP_ADMIN(subscribeFlags);
	std::cerr << std::endl;
	std::cerr << std::endl;
#endif

	QAction *action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe to Forum"), this, SLOT(subscribeToForum()));
	action->setDisabled (mForumId.empty() || IS_GROUP_SUBSCRIBED(subscribeFlags));

	action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe to Forum"), this, SLOT(unsubscribeToForum()));
	action->setEnabled (!mForumId.empty() && IS_GROUP_SUBSCRIBED(subscribeFlags));

	if (!Settings->getForumOpenAllInNewTab()) {
		action = contextMnu.addAction(QIcon(""), tr("Open in new tab"), this, SLOT(openInNewTab()));
		if (mForumId.empty() || forumThreadWidget(mForumId)) {
			action->setEnabled(false);
		}
	}

	contextMnu.addSeparator();

	contextMnu.addAction(QIcon(IMAGE_NEWFORUM), tr("New Forum"), this, SLOT(newforum()));

	action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Forum Details"), this, SLOT(showForumDetails()));
	action->setEnabled (!mForumId.empty ());

	action = contextMnu.addAction(QIcon(":/images/settings16.png"), tr("Edit Forum Details"), this, SLOT(editForumDetails()));
	action->setEnabled (!mForumId.empty () && IS_GROUP_ADMIN(subscribeFlags));

	QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Forum"), &contextMnu);
	connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );
	shareKeyAct->setEnabled(!mForumId.empty() && IS_GROUP_ADMIN(subscribeFlags));
	contextMnu.addAction( shareKeyAct);

	QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Forum" ), &contextMnu);
	connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreForumKeys() ) );
	restoreKeysAct->setEnabled(!mForumId.empty() && !IS_GROUP_ADMIN(subscribeFlags));
	contextMnu.addAction( restoreKeysAct);

	action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyForumLink()));
	action->setEnabled(!mForumId.empty());

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(":/images/message-mail-read.png"), tr("Mark all as read"), this, SLOT(markMsgAsRead()));
	action->setEnabled (!mForumId.empty () && IS_GROUP_SUBSCRIBED(subscribeFlags));

	action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark all as unread"), this, SLOT(markMsgAsUnread()));
	action->setEnabled (!mForumId.empty () && IS_GROUP_SUBSCRIBED(subscribeFlags));

#ifdef DEBUG_FORUMS
	contextMnu.addSeparator();
	action = contextMnu.addAction("Generate mass data", this, SLOT(generateMassData()));
	action->setEnabled (!mCurrForumId.empty() && IS_GROUP_SUBSCRIBED(mSubscribeFlags));
#endif

	contextMnu.exec(QCursor::pos());
}

void GxsForumsDialog::restoreForumKeys(void)
{
	QMessageBox::warning(this, "RetroShare", "ToDo");

#ifdef TOGXS
	rsGxsForums->groupRestoreKeys(mCurrForumId);
#endif
}

void GxsForumsDialog::updateDisplay(bool /*initialFill*/)
{
	/* Update forums list */
	insertForums();
}

void GxsForumsDialog::forumInfoToGroupItemInfo(const RsGroupMetaData &forumInfo, GroupItemInfo &groupItemInfo)
{
	groupItemInfo.id = QString::fromStdString(forumInfo.mGroupId);
	groupItemInfo.name = QString::fromUtf8(forumInfo.mGroupName.c_str());
	//groupItemInfo.description = QString::fromUtf8(forumInfo.forumDesc);
	groupItemInfo.popularity = forumInfo.mPop;
	groupItemInfo.lastpost = QDateTime::fromTime_t(forumInfo.mLastPost);
	groupItemInfo.subscribeFlags = forumInfo.mSubscribeFlags;

#if TOGXS
	if (forumInfo.mGroupFlags & RS_DISTRIB_AUTHEN_REQ) {
		groupItemInfo.name += " (" + tr("AUTHD") + ")";
		groupItemInfo.icon = QIcon(IMAGE_FORUMAUTHD);
	}
	else
#endif
	{
		groupItemInfo.icon = QIcon(IMAGE_FORUM);
	}
}

/***** INSERT FORUM LISTS *****/
void GxsForumsDialog::insertForumsData(const std::list<RsGroupMetaData> &forumList)
{
	std::list<RsGroupMetaData>::const_iterator it;

	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for (it = forumList.begin(); it != forumList.end(); it++) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->mSubscribeFlags;

		GroupItemInfo groupItemInfo;
		forumInfoToGroupItemInfo(*it, groupItemInfo);

		if (IS_GROUP_SUBSCRIBED(flags)) 
		{
			if (IS_GROUP_ADMIN(flags)) 
			{
				adminList.push_back(groupItemInfo);
			}
			else
			{
				/* subscribed forum */
				subList.push_back(groupItemInfo);
			}
		} 
		else 
		{
			/* rate the others by popularity */
			popMap.insert(std::make_pair(it->mPop, groupItemInfo));
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10)
	{
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	uint32_t popLimit = 0;
	std::multimap<uint32_t, GroupItemInfo>::reverse_iterator rit;
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); rit++, i++) ;
	if (rit != popMap.rend()) {
		popLimit = rit->first;
	}

	for (rit = popMap.rbegin(); rit != popMap.rend(); rit++) {
		if (rit->second.popularity < (int) popLimit) {
			otherList.append(rit->second);
		} else {
			popList.append(rit->second);
		}
	}

	/* now we can add them in as a tree! */
	ui.forumTreeWidget->fillGroupItems(yourForums, adminList);
	ui.forumTreeWidget->fillGroupItems(subscribedForums, subList);
	ui.forumTreeWidget->fillGroupItems(popularForums, popList);
	ui.forumTreeWidget->fillGroupItems(otherForums, otherList);

	updateMessageSummaryList("");
}

GxsForumThreadWidget *GxsForumsDialog::forumThreadWidget(const std::string &forumId)
{
	int tabCount = ui.threadTabWidget->count();
	for (int index = 0; index < tabCount; ++index) {
		GxsForumThreadWidget *childWidget = dynamic_cast<GxsForumThreadWidget*>(ui.threadTabWidget->widget(index));
		if (mThreadWidget && childWidget == mThreadWidget) {
			continue;
		}
		if (childWidget && childWidget->forumId() == forumId) {
			return childWidget;
			break;
		}
	}

	return NULL;
}

GxsForumThreadWidget *GxsForumsDialog::createThreadWidget(const std::string &forumId)
{
	GxsForumThreadWidget *threadWidget = new GxsForumThreadWidget(forumId);
	int index = ui.threadTabWidget->addTab(threadWidget, threadWidget->forumName(true));
	ui.threadTabWidget->setTabIcon(index, threadWidget->forumIcon());
	connect(threadWidget, SIGNAL(forumChanged(QWidget*)), this, SLOT(threadTabInfoChanged(QWidget*)));

	return threadWidget;
}

void GxsForumsDialog::changedForum(const QString &forumId)
{
	mForumId = forumId.toStdString();
	if (mForumId.empty()) {
		return;
	}

//	requestGroupSummary_CurrentForum(mForumId);

	/* search exisiting tab */
	GxsForumThreadWidget *threadWidget = forumThreadWidget(mForumId);

	if (!threadWidget) {
		if (mThreadWidget) {
			/* not found, use standard tab */
			threadWidget = mThreadWidget;
			threadWidget->setForumId(mForumId);
		} else {
			/* create new tab */
			threadWidget = createThreadWidget(mForumId);
		}
	}

	ui.threadTabWidget->setCurrentWidget(threadWidget);
}

void GxsForumsDialog::forumTreeMiddleButtonClicked(QTreeWidgetItem *item)
{
	openForumInNewTab(ui.forumTreeWidget->itemId(item).toStdString());
}

void GxsForumsDialog::openInNewTab()
{
	openForumInNewTab(mForumId);
}

void GxsForumsDialog::openForumInNewTab(const std::string &forumId)
{
	if (forumId.empty()) {
		return;
	}

	/* search exisiting tab */
	GxsForumThreadWidget *threadWidget = forumThreadWidget(forumId);
	if (!threadWidget) {
		/* not found, create new tab */
		threadWidget = createThreadWidget(forumId);
	}

	ui.threadTabWidget->setCurrentWidget(threadWidget);
}

void GxsForumsDialog::threadTabCloseRequested(int index)
{
	GxsForumThreadWidget *threadWidget = dynamic_cast<GxsForumThreadWidget*>(ui.threadTabWidget->widget(index));
	if (!threadWidget) {
		return;
	}

	if (threadWidget == mThreadWidget) {
		return;
	}

	delete(threadWidget);
}

void GxsForumsDialog::threadTabChanged(int index)
{
	GxsForumThreadWidget *threadWidget = dynamic_cast<GxsForumThreadWidget*>(ui.threadTabWidget->widget(index));
	if (!threadWidget) {
		return;
	}

	ui.forumTreeWidget->activateId(QString::fromStdString(threadWidget->forumId()), false);
}

void GxsForumsDialog::threadTabInfoChanged(QWidget *widget)
{
	int index = ui.threadTabWidget->indexOf(widget);
	if (index < 0) {
		return;
	}

	GxsForumThreadWidget *threadWidget = dynamic_cast<GxsForumThreadWidget*>(ui.threadTabWidget->widget(index));
	if (!threadWidget) {
		return;
	}

	ui.threadTabWidget->setTabText(index, threadWidget->forumName(true));
	ui.threadTabWidget->setTabIcon(index, threadWidget->forumIcon());
}

void GxsForumsDialog::copyForumLink()
{
	if (mForumId.empty()) {
		return;
	}

// THIS CODE CALLS getForumInfo() to verify that the Ids are valid.
// As we are switching to Request/Response this is now harder to do...
// So not bothering any more - shouldn't be necessary.
// IF we get errors - fix them, rather than patching here.
#if 0
	ForumInfo fi;
	if (rsGxsForums->getForumInfo(mCurrForumId, fi)) {
		RetroShareLink link;
		if (link.createForum(fi.forumId, "")) {
			QList<RetroShareLink> urls;
			urls.push_back(link);
			RSLinkClipboard::copyLinks(urls);
		}
	}
#endif

	QMessageBox::warning(this, "RetroShare", "ToDo");
}

void GxsForumsDialog::markMsgAsRead()
{
	GxsForumThreadWidget *threadWidget = forumThreadWidget(mForumId);
	if (threadWidget) {
		threadWidget->setAllMsgReadStatus(true);
	}
}

void GxsForumsDialog::markMsgAsUnread()
{
	GxsForumThreadWidget *threadWidget = forumThreadWidget(mForumId);
	if (threadWidget) {
		threadWidget->setAllMsgReadStatus(false);
	}
}

void GxsForumsDialog::newforum()
{
	GxsForumGroupDialog cf(mForumQueue, this);
	cf.exec ();
}

void GxsForumsDialog::subscribeToForum()
{
	forumSubscribe(true);
}

void GxsForumsDialog::unsubscribeToForum()
{
	forumSubscribe(false);
}

void GxsForumsDialog::forumSubscribe(bool subscribe)
{
	if (mForumId.empty()) {
		return;
	}

	uint32_t token;
	rsGxsForums->subscribeToGroup(token, mForumId, subscribe);
	mForumQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

void GxsForumsDialog::showForumDetails()
{
	if (mForumId.empty()) {
		return;
	}

	RsGxsForumGroup grp;
	grp.mMeta.mGroupId = mForumId;

	GxsForumGroupDialog cf(grp, GxsGroupDialog::MODE_SHOW, this);
	cf.exec ();
}

void GxsForumsDialog::editForumDetails()
{
	if (mForumId.empty()) {
		return;
	}

	RsGxsForumGroup grp;
	grp.mMeta.mGroupId = mForumId;

	GxsForumGroupDialog cf(grp, GxsGroupDialog::MODE_EDIT, this);
	cf.exec ();
}

void GxsForumsDialog::shareKey()
{
	ShareKey shareUi(this, mForumId, FORUM_KEY_SHARE);
	shareUi.exec();
}

void GxsForumsDialog::updateMessageSummaryList(std::string forumId)
{
	QTreeWidgetItem *items[2] = { yourForums, subscribedForums };

	for (int item = 0; item < 2; item++) {
		int child;
		int childCount = items[item]->childCount();
		for (child = 0; child < childCount; child++) {
			QTreeWidgetItem *childItem = items[item]->child(child);
			std::string childId = ui.forumTreeWidget->itemId(childItem).toStdString();
			if (childId.empty()) {
				continue;
			}

			if (forumId.empty() || childId == forumId) {
				/* calculate unread messages */
				unsigned int newMessageCount = 0;
				unsigned int unreadMessageCount = 0;

//#TODO				rsGxsForums->getMessageCount(childId, newMessageCount, unreadMessageCount);

				std::cerr << "IMPLEMENT rsGxsForums->getMessageCount()";
				std::cerr << std::endl;

				ui.forumTreeWidget->setUnreadCount(childItem, unreadMessageCount);

				if (forumId.empty() == false) {
					/* Calculate only this forum */
					break;
				}
			}
		}
	}
}

bool GxsForumsDialog::navigate(const std::string& forumId, const std::string& msgId)
{
	if (forumId.empty()) {
		return false;
	}

	if (ui.forumTreeWidget->activateId(QString::fromStdString(forumId), msgId.empty()) == NULL) {
		return false;
	}

	/* Threads are filled in changedForum */
	if (mForumId != forumId) {
		return false;
	}

	if (msgId.empty()) {
		return true;
	}

//#TODO
//	if (mThreadLoading) {
//		mThreadLoad.FocusMsgId = msgId;
//		return true;
//	}

	/* Search exisiting item */
//	QTreeWidgetItemIterator itemIterator(ui.threadTreeWidget);
//	QTreeWidgetItem *item = NULL;
//	while ((item = *itemIterator) != NULL) {
//		itemIterator++;

//		if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString() == msgId) {
//			ui.threadTreeWidget->setCurrentItem(item);
//			ui.threadTreeWidget->setFocus();
//			return true;
//		}
//	}

	return false;
}

void GxsForumsDialog::generateMassData()
{
#ifdef DEBUG_FORUMS
	if (mCurrForumId.empty ()) {
		return;
	}

	if (QMessageBox::question(this, "Generate mass data", "Do you really want to generate mass data ?", QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
		return;
	}

	for (int thread = 1; thread < 1000; thread++) {
		ForumMsgInfo threadInfo;
		threadInfo.forumId = mCurrForumId;
		threadInfo.title = QString("Test %1").arg(thread, 3, 10, QChar('0')).toStdWString();
		threadInfo.msg = QString("That is only a test").toStdWString();

		if (rsGxsForums->ForumMessageSend(threadInfo) == false) {
			return;
		}

		for (int msg = 1; msg < 3; msg++) {
			ForumMsgInfo msgInfo;
			msgInfo.forumId = mCurrForumId;
			msgInfo.threadId = threadInfo.msgId;
			msgInfo.parentId = threadInfo.msgId;
			msgInfo.title = threadInfo.title;
			msgInfo.msg = threadInfo.msg;

			if (rsGxsForums->ForumMessageSend(msgInfo) == false) {
				return;
			}
		}
	}
#endif
}

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

void GxsForumsDialog::insertForums()
{
	requestGroupSummary();
}

void GxsForumsDialog::requestGroupSummary()
{
//	mStateHelper->setLoading(TOKEN_TYPE_LISTING, true);

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::requestGroupSummary()";
	std::cerr << std::endl;
#endif

	mForumQueue->cancelActiveRequestTokens(TOKEN_TYPE_LISTING);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mForumQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_TYPE_LISTING);
}

void GxsForumsDialog::loadGroupSummary(const uint32_t &token)
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::loadGroupSummary()";
	std::cerr << std::endl;
#endif

	std::list<RsGroupMetaData> groupInfo;
	rsGxsForums->getGroupSummary(token, groupInfo);

	if (groupInfo.size() > 0)
	{
//		mStateHelper->setActive(TOKEN_TYPE_LISTING, true);

		insertForumsData(groupInfo);
	}
	else
	{
		std::cerr << "GxsForumsDialog::loadGroupSummary() ERROR No Groups...";
		std::cerr << std::endl;

//		mStateHelper->setActive(TOKEN_TYPE_LISTING, false);
	}

//	mStateHelper->setLoading(TOKEN_TYPE_LISTING, false);
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumsDialog::acknowledgeSubscribeChange(const uint32_t &token)
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::acknowledgeSubscribeChange()";
	std::cerr << std::endl;
#endif

	RsGxsGroupId groupId;
	rsGxsForums->acknowledgeGrp(token, groupId);

	insertForums();
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

//void GxsForumsDialog::requestGroupSummary_CurrentForum(const std::string &forumId)
//{
//	RsTokReqOptions opts;
//	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

//	std::list<std::string> grpIds;
//	grpIds.push_back(forumId);

//	std::cerr << "GxsForumsDialog::requestGroupSummary_CurrentForum(" << forumId << ")";
//	std::cerr << std::endl;

//	uint32_t token;
//	mForumQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_CURRENTFORUM);
//}

//void GxsForumsDialog::loadGroupSummary_CurrentForum(const uint32_t &token)
//{
//	std::cerr << "GxsForumsDialog::loadGroupSummary_CurrentForum()";
//	std::cerr << std::endl;

//	std::list<RsGroupMetaData> groupInfo;
//	rsGxsForums->getGroupSummary(token, groupInfo);

//	if (groupInfo.size() == 1)
//	{
//		RsGroupMetaData fi = groupInfo.front();
//		mSubscribeFlags = fi.mSubscribeFlags;
//	}
//	else
//	{
//		resetData();
//		std::cerr << "GxsForumsDialog::loadGroupSummary_CurrentForum() ERROR Invalid Number of Groups...";
//		std::cerr << std::endl;
//	}

//	setValid(true);
//}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumsDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	if (queue == mForumQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
		case TOKEN_TYPE_LISTING:
			loadGroupSummary(req.mToken);
			break;

		case TOKEN_TYPE_SUBSCRIBE_CHANGE:
			acknowledgeSubscribeChange(req.mToken);
			break;

//		case TOKEN_TYPE_CURRENTFORUM:
//			loadGroupSummary_CurrentForum(req.mToken);
//			break;

		default:
			std::cerr << "GxsForumsDialog::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;
		}
	}
}
