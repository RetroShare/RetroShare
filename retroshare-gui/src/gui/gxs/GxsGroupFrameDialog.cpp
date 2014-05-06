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
#include <QToolButton>

#include "GxsGroupFrameDialog.h"
#include "ui_GxsGroupFrameDialog.h"
#include "GxsMessageFrameWidget.h"

#include "gui/settings/rsharesettings.h"
#include "gui/RetroShareLink.h"
#include "gui/channels/ShareKey.h"
#include "gui/common/RSTreeWidget.h"
#include "gui/notifyqt.h"
//#include "gui/common/UIStateHelper.h"

//#define DEBUG_GROUPFRAMEDIALOG

/* Images for TreeWidget */
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
//#define IMAGE_GROUPAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"
#define IMAGE_EDIT           ":/images/edit_16.png"
#define IMAGE_SHARE           ":/images/share-icon-16.png"
#define IMAGE_TABNEW           ":/images/tab-new.png"

#define TOKEN_TYPE_LISTING          1
#define TOKEN_TYPE_SUBSCRIBE_CHANGE 2
//#define TOKEN_TYPE_CURRENTGROUP     3

/*
 * Transformation Notes:
 *   there are still a couple of things that the new groups differ from Old version.
 *   these will need to be addressed in the future.
 *     -> Child TS (for sorting) is not handled by GXS, this will probably have to be done in the GUI.
 *     -> Need to handle IDs properly.
 *     -> Popularity not handled in GXS yet.
 *     -> Much more to do.
 */

/** Constructor */
GxsGroupFrameDialog::GxsGroupFrameDialog(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
: RsGxsUpdateBroadcastPage(ifaceImpl, parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::GxsGroupFrameDialog();
	ui->setupUi(this);

	mInitialized = false;
	mYourGroups = NULL;
	mSubscribedGroups = NULL;
	mPopularGroups = NULL;
	mOtherGroups = NULL;
	mMessageWidget = NULL;

	/* Setup Queue */
	mInterface = ifaceImpl;
	mTokenQueue = new TokenQueue(mInterface->getTokenService(), this);

	/* Setup UI helper */
//	mStateHelper = new UIStateHelper(this);
	// no widget to add yet

	connect(ui->groupTreeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(groupTreeCustomPopupMenu(QPoint)));
	connect(ui->groupTreeWidget, SIGNAL(treeItemActivated(QString)), this, SLOT(changedGroup(QString)));
	connect(ui->groupTreeWidget->treeWidget(), SIGNAL(signalMouseMiddleButtonClicked(QTreeWidgetItem*)), this, SLOT(groupTreeMiddleButtonClicked(QTreeWidgetItem*)));
	connect(ui->messageTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(messageTabCloseRequested(int)));
	connect(ui->messageTabWidget, SIGNAL(currentChanged(int)), this, SLOT(messageTabChanged(int)));

	connect(ui->todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));

	/* Set initial size the splitter */
	QList<int> sizes;
	sizes << 300 << width(); // Qt calculates the right sizes
	ui->splitter->setSizes(sizes);
}

GxsGroupFrameDialog::~GxsGroupFrameDialog()
{
	// save settings
	processSettings(false);

	delete(mTokenQueue);
	delete(ui);
}

void GxsGroupFrameDialog::initUi()
{
	ui->titleBarPixmap->setPixmap(QPixmap(icon(ICON_NAME)));
	ui->titleBarLabel->setText(text(TEXT_NAME));

	/* Initialize group tree */
	QToolButton *newGroupButton = new QToolButton(this);
	newGroupButton->setIcon(QIcon(icon(ICON_NEW)));
	newGroupButton->setToolTip(text(TEXT_NEW));
	connect(newGroupButton, SIGNAL(clicked()), this, SLOT(newGroup()));
	ui->groupTreeWidget->addToolButton(newGroupButton);

	/* Create group tree */
	mYourGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_YOUR_GROUP), QIcon(icon(ICON_YOUR_GROUP)), true);
	mSubscribedGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_SUBSCRIBED_GROUP), QIcon(icon(ICON_SUBSCRIBED_GROUP)), true);
	mPopularGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_POPULAR_GROUP), QIcon(icon(ICON_POPULAR_GROUP)), false);
	mOtherGroups = ui->groupTreeWidget->addCategoryItem(text(TEXT_OTHER_GROUP), QIcon(icon(ICON_OTHER_GROUP)), false);

	if (text(TEXT_TODO).isEmpty()) {
		ui->todoPushButton->hide();
	}

	// load settings
	mSettingsName = settingsGroupName();
	processSettings(true);
}

void GxsGroupFrameDialog::showEvent(QShowEvent *event)
{
	if (!mInitialized) {
		/* Problem: virtual methods cannot be used in constructor */
		mInitialized = true;

		initUi();
	}

	RsGxsUpdateBroadcastPage::showEvent(event);
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

	ui->groupTreeWidget->processSettings(Settings, load);

	Settings->endGroup();
}

void GxsGroupFrameDialog::setSingleTab(bool singleTab)
{
	if (singleTab) {
		if (!mMessageWidget) {
			mMessageWidget = createMessageWidget(RsGxsGroupId());
			// remove close button of the the first tab
			ui->messageTabWidget->hideCloseButton(ui->messageTabWidget->indexOf(mMessageWidget));
		}
	} else {
		if (mMessageWidget) {
			delete(mMessageWidget);
			mMessageWidget = NULL;
		}
	}
}

void GxsGroupFrameDialog::updateDisplay(bool complete)
{
	if (complete || !getGrpIds().empty()) {
		/* Update group list */
		requestGroupSummary();
	}
}

void GxsGroupFrameDialog::todo()
{
	QMessageBox::information(this, "Todo", text(TEXT_TODO));
}

void GxsGroupFrameDialog::groupTreeCustomPopupMenu(QPoint /*point*/)
{
	int subscribeFlags = ui->groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId.toStdString()));

	bool isAdmin = IS_GROUP_ADMIN(subscribeFlags);
	bool isPublisher = IS_GROUP_PUBLISHER(subscribeFlags);
	bool isSubscribed = IS_GROUP_SUBSCRIBED(subscribeFlags);

	QMenu contextMnu(this);

	QAction *action;

	if (isSubscribed) {
		action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe"), this, SLOT(unsubscribeGroup()));
		action->setEnabled (!mGroupId.isNull() && IS_GROUP_SUBSCRIBED(subscribeFlags));
	} else {
		action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe"), this, SLOT(subscribeGroup()));
		action->setDisabled (mGroupId.isNull() || IS_GROUP_SUBSCRIBED(subscribeFlags));
	}

	if (mMessageWidget) {
		action = contextMnu.addAction(QIcon(IMAGE_TABNEW), tr("Open in new tab"), this, SLOT(openInNewTab()));
		if (mGroupId.isNull() || messageWidget(mGroupId, true)) {
			action->setEnabled(false);
		}
	}

	contextMnu.addSeparator();

	contextMnu.addAction(QIcon(icon(ICON_NEW)), text(TEXT_NEW), this, SLOT(newGroup()));

	action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Details"), this, SLOT(showGroupDetails()));
	action->setEnabled (!mGroupId.isNull());

	action = contextMnu.addAction(QIcon(IMAGE_EDIT), tr("Edit Details"), this, SLOT(editGroupDetails()));
	action->setEnabled (!mGroupId.isNull() && isAdmin);

	action = contextMnu.addAction(QIcon(IMAGE_SHARE), tr("Share"), this, SLOT(shareKey()));
	action->setEnabled(!mGroupId.isNull() && isAdmin);

	if (!mGroupId.isNull() && isPublisher && !isAdmin) {
		contextMnu.addAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights" ), this, SLOT(restoreGroupKeys()));
	}

	action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyGroupLink()));
	action->setEnabled(!mGroupId.isNull());

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(":/images/message-mail-read.png"), tr("Mark all as read"), this, SLOT(markMsgAsRead()));
	action->setEnabled (!mGroupId.isNull() && isSubscribed);

	action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark all as unread"), this, SLOT(markMsgAsUnread()));
	action->setEnabled (!mGroupId.isNull() && isSubscribed);

	/* Add special actions */
	QList<QAction*> actions;
	groupTreeCustomActions(mGroupId, subscribeFlags, actions);
	if (!actions.isEmpty()) {
		contextMnu.addSeparator();
		contextMnu.addActions(actions);
	}

	contextMnu.exec(QCursor::pos());
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
	GxsGroupDialog *dialog = createNewGroupDialog(mTokenQueue);
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
	mTokenQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

void GxsGroupFrameDialog::showGroupDetails()
{
	if (mGroupId.isNull()) {
		return;
	}

	GxsGroupDialog *dialog = createGroupDialog(mTokenQueue, mInterface->getTokenService(), GxsGroupDialog::MODE_SHOW, mGroupId);
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

	GxsGroupDialog *dialog = createGroupDialog(mTokenQueue, mInterface->getTokenService(), GxsGroupDialog::MODE_EDIT, mGroupId);
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

void GxsGroupFrameDialog::markMsgAsRead()
{
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId, false);
	if (msgWidget) {
		msgWidget->setAllMessagesRead(true);
	}
}

void GxsGroupFrameDialog::markMsgAsUnread()
{
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId, false);
	if (msgWidget) {
		msgWidget->setAllMessagesRead(false);
	}
}

void GxsGroupFrameDialog::shareKey()
{
	if (mGroupId.isNull()) {
		return;
	}

	ShareKey shareUi(this, mGroupId.toStdString(), shareKeyType());
	shareUi.exec();
}

bool GxsGroupFrameDialog::navigate(const RsGxsGroupId groupId, const std::string& msgId)
{
	if (groupId.isNull()) {
		return false;
	}

	if (ui->groupTreeWidget->activateId(QString::fromStdString(groupId.toStdString()), msgId.empty()) == NULL) {
		return false;
	}

	if (mGroupId != groupId) {
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
//	QTreeWidgetItemIterator itemIterator(ui->threadTreeWidget);
//	QTreeWidgetItem *item = NULL;
//	while ((item = *itemIterator) != NULL) {
//		itemIterator++;

//		if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString() == msgId) {
//			ui->threadTreeWidget->setCurrentItem(item);
//			ui->threadTreeWidget->setFocus();
//			return true;
//		}
//	}

	return false;
}

GxsMessageFrameWidget *GxsGroupFrameDialog::messageWidget(const RsGxsGroupId &groupId, bool ownTab)
{
	int tabCount = ui->messageTabWidget->count();
	for (int index = 0; index < tabCount; ++index) {
		GxsMessageFrameWidget *childWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
		if (ownTab && mMessageWidget && childWidget == mMessageWidget) {
			continue;
		}
		if (childWidget && childWidget->groupId() == groupId) {
			return childWidget;
		}
	}

	return NULL;
}

GxsMessageFrameWidget *GxsGroupFrameDialog::createMessageWidget(const RsGxsGroupId &groupId)
{
	GxsMessageFrameWidget *msgWidget = createMessageFrameWidget(groupId);
	if (!msgWidget) {
		return NULL;
	}

	int index = ui->messageTabWidget->addTab(msgWidget, msgWidget->groupName(true));
	ui->messageTabWidget->setTabIcon(index, msgWidget->groupIcon());
	connect(msgWidget, SIGNAL(groupChanged(QWidget*)), this, SLOT(messageTabInfoChanged(QWidget*)));

	return msgWidget;
}

void GxsGroupFrameDialog::changedGroup(const QString &groupId)
{
	mGroupId = RsGxsGroupId(groupId.toStdString());
	if (mGroupId.isNull()) {
		return;
	}

	/* search exisiting tab */
	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId, true);

	if (!msgWidget) {
		if (mMessageWidget) {
			/* not found, use standard tab */
			msgWidget = mMessageWidget;
			msgWidget->setGroupId(mGroupId);
		} else {
			/* create new tab */
			msgWidget = createMessageWidget(mGroupId);
		}
	}

	ui->messageTabWidget->setCurrentWidget(msgWidget);
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
	GxsMessageFrameWidget *msgWidget = messageWidget(groupId, true);
	if (!msgWidget) {
		/* not found, create new tab */
		msgWidget = createMessageWidget(groupId);
	}

	ui->messageTabWidget->setCurrentWidget(msgWidget);
}

void GxsGroupFrameDialog::messageTabCloseRequested(int index)
{
	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
	if (!msgWidget) {
		return;
	}

	if (msgWidget == mMessageWidget) {
		return;
	}

	delete(msgWidget);
}

void GxsGroupFrameDialog::messageTabChanged(int index)
{
	GxsMessageFrameWidget *msgWidget = dynamic_cast<GxsMessageFrameWidget*>(ui->messageTabWidget->widget(index));
	if (!msgWidget) {
		return;
	}

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

///***** INSERT GROUP LISTS *****/
void GxsGroupFrameDialog::groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo)
{
	groupItemInfo.id = QString::fromStdString(groupInfo.mGroupId.toStdString());
	groupItemInfo.name = QString::fromUtf8(groupInfo.mGroupName.c_str());
	//groupItemInfo.description =
	groupItemInfo.popularity = groupInfo.mPop;
	groupItemInfo.lastpost = QDateTime::fromTime_t(groupInfo.mLastPost);
	groupItemInfo.subscribeFlags = groupInfo.mSubscribeFlags;

#if TOGXS
	if (groupInfo.mGroupFlags & RS_DISTRIB_AUTHEN_REQ) {
		groupItemInfo.name += " (" + tr("AUTHD") + ")";
		groupItemInfo.icon = QIcon(IMAGE_GROUPAUTHD);
	}
	else
#endif
	{
		groupItemInfo.icon = QIcon(icon(ICON_DEFAULT));
	}
}

void GxsGroupFrameDialog::insertGroupsData(const std::list<RsGroupMetaData> &groupList)
{
	if (!mInitialized) {
		return;
	}

	std::list<RsGroupMetaData>::const_iterator it;

	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for (it = groupList.begin(); it != groupList.end(); it++) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->mSubscribeFlags;

		GroupItemInfo groupItemInfo;
		groupInfoToGroupItemInfo(*it, groupItemInfo);

		if (IS_GROUP_SUBSCRIBED(flags))
		{
			if (IS_GROUP_ADMIN(flags))
			{
				adminList.push_back(groupItemInfo);
			}
			else
			{
				/* subscribed group */
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
	ui->groupTreeWidget->fillGroupItems(mYourGroups, adminList);
	ui->groupTreeWidget->fillGroupItems(mSubscribedGroups, subList);
	ui->groupTreeWidget->fillGroupItems(mPopularGroups, popList);
	ui->groupTreeWidget->fillGroupItems(mOtherGroups, otherList);

	updateMessageSummaryList(RsGxsGroupId());
}

void GxsGroupFrameDialog::updateMessageSummaryList(RsGxsGroupId groupId)
{
	if (!mInitialized) {
		return;
	}

	QTreeWidgetItem *items[2] = { mYourGroups, mSubscribedGroups };

	for (int item = 0; item < 2; item++) {
		int child;
		int childCount = items[item]->childCount();
		for (child = 0; child < childCount; child++) {
			QTreeWidgetItem *childItem = items[item]->child(child);
			std::string childId = ui->groupTreeWidget->itemId(childItem).toStdString();
			if (childId.empty()) {
				continue;
			}

			if (groupId.isNull() || childId == groupId.toStdString()) {
				/* calculate unread messages */
				unsigned int newMessageCount = 0;
				unsigned int unreadMessageCount = 0;

//#TODO				mInterface->getMessageCount(childId, newMessageCount, unreadMessageCount);

				std::cerr << "IMPLEMENT mInterface->getMessageCount()";
				std::cerr << std::endl;

				ui->groupTreeWidget->setUnreadCount(childItem, unreadMessageCount);

				if (groupId.isNull() == false) {
					/* Calculate only this group */
					break;
				}
			}
		}
	}
}

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

void GxsGroupFrameDialog::requestGroupSummary()
{
//	mStateHelper->setLoading(TOKEN_TYPE_LISTING, true);

#ifdef DEBUG_GROUPFRAMEDIALOG
	std::cerr << "GxsGroupFrameDialog::requestGroupSummary()";
	std::cerr << std::endl;
#endif

	mTokenQueue->cancelActiveRequestTokens(TOKEN_TYPE_LISTING);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_TYPE_LISTING);
}

void GxsGroupFrameDialog::loadGroupSummary(const uint32_t &token)
{
#ifdef DEBUG_GROUPFRAMEDIALOG
	std::cerr << "GxsGroupFrameDialog::loadGroupSummary()";
	std::cerr << std::endl;
#endif

	std::list<RsGroupMetaData> groupInfo;
	mInterface->getGroupSummary(token, groupInfo);

	if (groupInfo.size() > 0)
	{
//		mStateHelper->setActive(TOKEN_TYPE_LISTING, true);

		insertGroupsData(groupInfo);
	}
	else
	{
		std::cerr << "GxsGroupFrameDialog::loadGroupSummary() ERROR No Groups...";
		std::cerr << std::endl;

//		mStateHelper->setActive(TOKEN_TYPE_LISTING, false);
	}

//	mStateHelper->setLoading(TOKEN_TYPE_LISTING, false);
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsGroupFrameDialog::acknowledgeSubscribeChange(const uint32_t &token)
{
#ifdef DEBUG_GROUPFRAMEDIALOG
	std::cerr << "GxsGroupFrameDialog::acknowledgeSubscribeChange()";
	std::cerr << std::endl;
#endif

	RsGxsGroupId groupId;
	mInterface->acknowledgeGrp(token, groupId);

	fillComplete();
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

//void GxsGroupFrameDialog::requestGroupSummary_CurrentGroup(const RsGxsGroupId &groupId)
//{
//	RsTokReqOptions opts;
//	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

//	std::list<std::string> grpIds;
//	grpIds.push_back(groupId);

//	std::cerr << "GxsGroupFrameDialog::requestGroupSummary_CurrentGroup(" << groupId << ")";
//	std::cerr << std::endl;

//	uint32_t token;
//	mInteface->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_CURRENTGROUP);
//}

//void GxsGroupFrameDialog::loadGroupSummary_CurrentGroup(const uint32_t &token)
//{
//	std::cerr << "GxsGroupFrameDialog::loadGroupSummary_CurrentGroup()";
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
//		std::cerr << "GxsGroupFrameDialog::loadGroupSummary_CurrentGroup() ERROR Invalid Number of Groups...";
//		std::cerr << std::endl;
//	}

//	setValid(true);
//}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsGroupFrameDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_GROUPFRAMEDIALOG
	std::cerr << "GxsGroupFrameDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	if (queue == mTokenQueue)
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

//		case TOKEN_TYPE_CURRENTGROUP:
//			loadGroupSummary_CurrentGroup(req.mToken);
//			break;

		default:
			std::cerr << "GxsGroupFrameDialog::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;
		}
	}
}
